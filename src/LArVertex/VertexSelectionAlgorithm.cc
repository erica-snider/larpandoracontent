/**
 *  @file   LArContent/src/LArVertex/VertexSelectionAlgorithm.cc
 * 
 *  @brief  Implementation of the vertex selection algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "LArHelpers/LArClusterHelper.h"
#include "LArHelpers/LArGeometryHelper.h"

#include "LArVertex/VertexSelectionAlgorithm.h"

using namespace pandora;

namespace lar_content
{

VertexSelectionAlgorithm::VertexSelectionAlgorithm() :
    m_replaceCurrentVertexList(true),
    m_histogramNPhiBins(200),
    m_histogramPhiMin(-1.1f * M_PI),
    m_histogramPhiMax(+1.1f * M_PI),
    m_maxHitVertexDisplacement(std::numeric_limits<float>::max()),
    m_maxOnHitDisplacement(1.f),
    m_hitDeweightingPower(-0.5f),
    m_maxTopScoreCandidates(5),
    m_minCandidateDisplacement(2.f),
    m_minCandidateScoreFraction(0.9f)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode VertexSelectionAlgorithm::Run()
{
    const VertexList *pInputVertexList(NULL);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pInputVertexList));

    VertexScoreList vertexScoreList;

    for (VertexList::const_iterator iter = pInputVertexList->begin(), iterEnd = pInputVertexList->end(); iter != iterEnd; ++iter)
    {
        Vertex *const pVertex(*iter);

        Histogram histogramU(m_histogramNPhiBins, m_histogramPhiMin, m_histogramPhiMax);
        Histogram histogramV(m_histogramNPhiBins, m_histogramPhiMin, m_histogramPhiMax);
        Histogram histogramW(m_histogramNPhiBins, m_histogramPhiMin, m_histogramPhiMax);

        const bool isVertexOnHitU(this->FillHistogram(pVertex, TPC_VIEW_U, m_inputClusterListNameU, histogramU));
        const bool isVertexOnHitV(this->FillHistogram(pVertex, TPC_VIEW_V, m_inputClusterListNameV, histogramV));
        const bool isVertexOnHitW(this->FillHistogram(pVertex, TPC_VIEW_W, m_inputClusterListNameW, histogramW));

        if (!isVertexOnHitU || !isVertexOnHitV || !isVertexOnHitW)
            continue;

        const float figureOfMerit(this->GetFigureOfMerit(histogramU, histogramV, histogramW));
        vertexScoreList.push_back(VertexScore(pVertex, figureOfMerit));
    }

    std::sort(vertexScoreList.begin(), vertexScoreList.end());

    unsigned int nVerticesConsidered(0);
    VertexScoreList selectedVertexScoreList;

    for (VertexScoreList::const_iterator iter = vertexScoreList.begin(), iterEnd = vertexScoreList.end(); iter != iterEnd; ++iter)
    {
        if (++nVerticesConsidered > m_maxTopScoreCandidates)
            break;

        if (!selectedVertexScoreList.empty() && !this->AcceptVertexLocation(iter->GetVertex(), selectedVertexScoreList))
            continue;

        if (!selectedVertexScoreList.empty() && !this->AcceptVertexScore(iter->GetScore(), selectedVertexScoreList))
            continue;

        selectedVertexScoreList.push_back(*iter);
    }

    if (!selectedVertexScoreList.empty())
    {
        Vertex *pBestVertex = vertexScoreList.at(0).GetVertex();
        VertexList selectedVertexList;
        selectedVertexList.insert(pBestVertex);

        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList(*this, m_outputVertexListName, selectedVertexList));

        if (m_replaceCurrentVertexList)
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ReplaceCurrentList<Vertex>(*this, m_outputVertexListName));
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

bool VertexSelectionAlgorithm::FillHistogram(const Vertex *const pVertex, const HitType hitType, const std::string &clusterListName,
    Histogram &histogram) const
{
    bool isVertexOnHit(false);
    const ClusterList *pClusterList = NULL;
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, clusterListName, pClusterList));

    for (ClusterList::const_iterator cIter = pClusterList->begin(), cIterEnd = pClusterList->end(); cIter != cIterEnd; ++cIter)
    {
        const Cluster *const pCluster(*cIter);
        const HitType clusterHitType(LArClusterHelper::GetClusterHitType(pCluster));

        if (hitType != clusterHitType)
            throw StatusCodeException(STATUS_CODE_INVALID_PARAMETER);

        const CartesianVector vertexPosition2D(LArGeometryHelper::ProjectPosition(this->GetPandora(), pVertex->GetPosition(), hitType));
        isVertexOnHit |= this->FillHistogram(vertexPosition2D, pCluster, histogram);
    }

    return isVertexOnHit;
}

//------------------------------------------------------------------------------------------------------------------------------------------

bool VertexSelectionAlgorithm::FillHistogram(const CartesianVector &vertexPosition2D, const Cluster *const pCluster, Histogram &histogram) const
{
    bool isVertexOnHit(false);
    const OrderedCaloHitList &orderedCaloHitList(pCluster->GetOrderedCaloHitList());

    for (OrderedCaloHitList::const_iterator iter = orderedCaloHitList.begin(), iterEnd = orderedCaloHitList.end(); iter != iterEnd; ++iter)
    {
        for (CaloHitList::const_iterator hIter = iter->second->begin(), hIterEnd = iter->second->end(); hIter != hIterEnd; ++hIter)
        {
            const CaloHit *const pCaloHit(*hIter);

            const CartesianVector displacement(pCaloHit->GetPositionVector() - vertexPosition2D);
            const float magnitude(displacement.GetMagnitude());

            if (magnitude > m_maxHitVertexDisplacement)
                continue;

            if (magnitude < m_maxOnHitDisplacement)
                isVertexOnHit = true;

            const float phi(std::atan2(displacement.GetZ(), displacement.GetX()));
            histogram.Fill(phi, std::pow(magnitude, m_hitDeweightingPower));
        }
    }

    return isVertexOnHit;
}

//------------------------------------------------------------------------------------------------------------------------------------------

float VertexSelectionAlgorithm::GetFigureOfMerit(const Histogram &histogramU, const Histogram &histogramV, const Histogram &histogramW) const
{
    float figureOfMerit(0.f);
    figureOfMerit += this->GetFigureOfMerit(histogramU);
    figureOfMerit += this->GetFigureOfMerit(histogramV);
    figureOfMerit += this->GetFigureOfMerit(histogramW);

    return figureOfMerit;
}

//------------------------------------------------------------------------------------------------------------------------------------------

float VertexSelectionAlgorithm::GetFigureOfMerit(const Histogram &histogram) const
{
    float sumSquaredEntries(0.f);

    for (int xBin = 0; xBin < histogram.GetNBinsX(); ++xBin)
    {
        const float binContent(histogram.GetBinContent(xBin));
        sumSquaredEntries += binContent * binContent;
    }

    return sumSquaredEntries;
}

//------------------------------------------------------------------------------------------------------------------------------------------

bool VertexSelectionAlgorithm::AcceptVertexLocation(const Vertex *const pVertex, const VertexScoreList &vertexScoreList) const
{
    const CartesianVector position(pVertex->GetPosition());

    for (VertexScoreList::const_iterator iter = vertexScoreList.begin(), iterEnd = vertexScoreList.end(); iter != iterEnd; ++iter)
    {
        const float displacement3D((position - iter->GetVertex()->GetPosition()).GetMagnitude());

        if (displacement3D < m_minCandidateDisplacement)
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------

bool VertexSelectionAlgorithm::AcceptVertexScore(const float score, const VertexScoreList &vertexScoreList) const
{
    for (VertexScoreList::const_iterator iter = vertexScoreList.begin(), iterEnd = vertexScoreList.end(); iter != iterEnd; ++iter)
    {
        if (score < m_minCandidateScoreFraction * iter->GetScore())
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode VertexSelectionAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameU", m_inputClusterListNameU));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameV", m_inputClusterListNameV));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameW", m_inputClusterListNameW));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputVertexListName", m_outputVertexListName));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ReplaceCurrentVertexList", m_replaceCurrentVertexList));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HistogramNPhiBins", m_histogramNPhiBins));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HistogramPhiMin", m_histogramPhiMin));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HistogramPhiMax", m_histogramPhiMax));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxHitVertexDisplacement", m_maxHitVertexDisplacement));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxOnHitDisplacement", m_maxOnHitDisplacement));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HitDeweightingPower", m_hitDeweightingPower));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTopScoreCandidates", m_maxTopScoreCandidates));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinCandidateDisplacement", m_minCandidateDisplacement));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinCandidateScoreFraction", m_minCandidateScoreFraction));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
