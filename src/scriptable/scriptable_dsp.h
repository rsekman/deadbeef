#ifndef scriptable_dsp_h
#define scriptable_dsp_h

#include "scriptable.h"
#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

scriptableItem_t *
scriptableDspRoot (void);

void
scriptableDspLoadPresets (void);

scriptableItem_t *
scriptableDspNodeItemFromDspContext (ddb_dsp_context_t *context);

scriptableItem_t *
scriptableDspPresetFromDspChain (ddb_dsp_context_t *chain);

ddb_dsp_context_t *
scriptableDspConfigToDspChain (scriptableItem_t *item);

#ifdef __cplusplus
}
#endif

#endif /* scriptable_dsp_h */
