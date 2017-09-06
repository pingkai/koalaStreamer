//
//  streamStatistician.h
//
//  Created by 平凯 on 2017/8/9.
//  Copyright © 2017年 PingKai. All rights reserved.
//

#ifndef stream_stasticsor_h
#define stream_stasticsor_h
// TODO : use a type header
#include "MediaStreamer.h"
#include "utils/mediaFrame.h"

#define MAX_STREAMES 8
typedef struct streamerStatistician_t streamerStatistician;

streamerStatistician *streamerStatisticianCreate();

int streamerStatistician_addStream(streamerStatistician *pHandle, Stream_type type);

void streamerStatistician_onFrameBegin(streamerStatistician *pHandle, int index, mediaFrame *frame);

void streamerStatistician_onFrameEnd(streamerStatistician *pHandle, int index, mediaFrame *frame);

uint32_t streamerStatistician_getStreamBitRate(streamerStatistician *pHandle, int index);
// factor = send data time / total time  in one statistician period on mux->send thread
float streamerStatistician_getStreamBandWideFactor(streamerStatistician *pHandle);
// start a new  statistician period
void streamerStatistician_reset(streamerStatistician *pHandle);
void streamerStatisticianRelease(streamerStatistician *pHandle);
#endif /* stream_stasticsor_h */
