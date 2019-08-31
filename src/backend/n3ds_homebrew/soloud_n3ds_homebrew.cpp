/*
SoLoud Nintendo 3DS homebrew output backend
Copyright (c) 2019 Felipe Izzo

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include <atomic>
#include <string.h>

#include "soloud.h"
#include "soloud_thread.h"

#if !defined(WITH_N3DS_HOMEBREW)

namespace SoLoud
{
	result n3ds_homebrew_init(Soloud *aSoloud, unsigned int aFlags, unsigned int aSamplerate, unsigned int aBuffer)
	{
		return NOT_IMPLEMENTED;
	}
};

#else

#include <3ds.h>
#include <stdio.h>

namespace SoLoud
{
	struct n3dsData {
		Soloud *soloud;
		ndspWaveBuf waveBuf[2];
		unsigned int waveBuf_id;
		unsigned int samples;
	};

	static void n3ds_cleanup(Soloud *aSoloud)
	{
		if (!aSoloud->mBackendData)
			return;

		n3dsData *data = (n3dsData*)aSoloud->mBackendData;

		ndspChnWaveBufClear(0);

		ndspExit();

		linearFree((void*)data->waveBuf[0].data_vaddr);
		linearFree((void*)data->waveBuf[1].data_vaddr);

		aSoloud->mBackendData = NULL;

		delete data;
	}

	static void n3ds_callback_func(void *arg)
	{
		n3dsData *data = (n3dsData*)arg;

		if(data->waveBuf[data->waveBuf_id].status == NDSP_WBUF_DONE)
		{
			data->soloud->mixSigned16(data->waveBuf[data->waveBuf_id].data_pcm16, data->samples);
			ndspChnWaveBufAdd(0, &data->waveBuf[data->waveBuf_id]);

			data->waveBuf_id = !data->waveBuf_id;
		}
	}

	result n3ds_homebrew_init(Soloud *aSoloud, unsigned int aFlags, unsigned int aSamplerate, unsigned int aBuffer, unsigned int aChannels)
	{
		if (aSamplerate != 44100 || aChannels != 2)
			return INVALID_PARAMETER;

		if (ndspInit() != 0)
			return 1;

		ndspSetOutputMode(NDSP_OUTPUT_STEREO);
		ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
		ndspChnSetRate(0, 44100.0f);

		n3dsData *data = new n3dsData;

		data->soloud = aSoloud;
		data->samples = aBuffer;

		data->waveBuf[0].data_vaddr = linearAlloc(aBuffer * 4);
		data->waveBuf[0].nsamples = aBuffer;
		data->waveBuf[1].data_vaddr = linearAlloc(aBuffer * 4);
		data->waveBuf[1].nsamples = aBuffer;

		memset(data->waveBuf[0].data_pcm16, 0, aBuffer * 4);
		memset(data->waveBuf[0].data_pcm16, 0, aBuffer * 4);

		data->waveBuf_id = 0;

		aSoloud->mBackendData = data;
		aSoloud->mBackendCleanupFunc = n3ds_cleanup;

		aSoloud->postinit(aSamplerate, data->samples * aChannels, aFlags, aChannels);

		ndspSetCallback(&n3ds_callback_func, (void*)data);

		ndspChnWaveBufAdd(0, &data->waveBuf[0]);
		ndspChnWaveBufAdd(0, &data->waveBuf[1]);

		return 0;
	}
};

#endif
