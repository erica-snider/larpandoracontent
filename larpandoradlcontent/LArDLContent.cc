/**
 *  @file   larpandoradlcontent/LArDLContent.cc
 *
 *  @brief  Factory implementations for content intended for use with particle flow reconstruction at liquid argon time projection chambers
 *
 *  $Log: $
 */

#include "Api/PandoraApi.h"

#include "Pandora/Algorithm.h"
#include "Pandora/AlgorithmTool.h"
#include "Pandora/Pandora.h"

#include "larpandoradlcontent/LArDeepLearning/DeepLearningTrackShowerIdAlgorithm.h"

#include "larpandoradlcontent/LArDLContent.h"

#define LAR_DL_ALGORITHM_LIST(d)                                                                                                   \
    d("LArDeepLearningTrackShowerId",           DeepLearningTrackShowerIdAlgorithm)

#define LAR_DL_ALGORITHM_TOOL_LIST(d)

#define DL_FACTORY Factory

//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------

namespace lar_dl_content
{

#define LAR_DL_CONTENT_CREATE_ALGORITHM_FACTORY(a, b)                                                                              \
class b##DL_FACTORY : public pandora::AlgorithmFactory                                                                             \
{                                                                                                                               \
public:                                                                                                                         \
    pandora::Algorithm *CreateAlgorithm() const {return new b;};                                                                \
};

LAR_DL_ALGORITHM_LIST(LAR_DL_CONTENT_CREATE_ALGORITHM_FACTORY)

//------------------------------------------------------------------------------------------------------------------------------------------

#define LAR_DL_CONTENT_CREATE_ALGORITHM_TOOL_FACTORY(a, b)                                                                         \
class b##DL_FACTORY : public pandora::AlgorithmToolFactory                                                                         \
{                                                                                                                               \
public:                                                                                                                         \
    pandora::AlgorithmTool *CreateAlgorithmTool() const {return new b;};                                                        \
};

LAR_DL_ALGORITHM_TOOL_LIST(LAR_DL_CONTENT_CREATE_ALGORITHM_TOOL_FACTORY)

} // namespace lar_dl_content

//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------

#define LAR_DL_CONTENT_REGISTER_ALGORITHM(a, b)                                                                                 \
{                                                                                                                               \
    const pandora::StatusCode statusCode(PandoraApi::RegisterAlgorithmFactory(pandora, a, new lar_dl_content::b##DL_FACTORY));  \
    if (pandora::STATUS_CODE_SUCCESS != statusCode)                                                                             \
        return statusCode;                                                                                                      \
}

#define LAR_DL_CONTENT_REGISTER_ALGORITHM_TOOL(a, b)                                                                                \
{                                                                                                                                   \
    const pandora::StatusCode statusCode(PandoraApi::RegisterAlgorithmToolFactory(pandora, a, new lar_dl_content::b##DL_FACTORY));  \
    if (pandora::STATUS_CODE_SUCCESS != statusCode)                                                                                 \
        return statusCode;                                                                                                          \
}

pandora::StatusCode LArDLContent::RegisterAlgorithms(const pandora::Pandora &pandora)
{
    LAR_DL_ALGORITHM_LIST(LAR_DL_CONTENT_REGISTER_ALGORITHM);
    LAR_DL_ALGORITHM_TOOL_LIST(LAR_DL_CONTENT_REGISTER_ALGORITHM_TOOL);
    return pandora::STATUS_CODE_SUCCESS;
}
