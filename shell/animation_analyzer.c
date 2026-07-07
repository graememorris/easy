#include <stdio.h>
#include <string.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "animation_analyzer.h"

int extract_animation_properties(const char *filepath, struct AnimationProperties *out_props)
{
	memset(out_props, 0, sizeof(struct AnimationProperties));
	AVFormatContext *format_ctx=NULL;
	if (avformat_open_input(&format_ctx, filepath, NULL, NULL)<0)	return 1;
	if (avformat_find_stream_info(format_ctx, NULL)<0){avformat_close_input(&format_ctx);	return 1;}
	int primary_video_idx=-1;
	for (unsigned int i=0; i<format_ctx->nb_streams; i++){
		if (format_ctx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
			out_props->video_stream_count++;
			if (primary_video_idx==-1) primary_video_idx=i;
		}
		else if (format_ctx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO) out_props->audio_stream_count++;
	}
	if (primary_video_idx==-1){avformat_close_input(&format_ctx); return 1;}

	AVStream *stream=format_ctx->streams[primary_video_idx];
	AVCodecParameters *codec_par=stream->codecpar;
	out_props->width=codec_par->width;
	out_props->height=codec_par->height;
	const AVCodec *codec=avcodec_find_decoder(codec_par->codec_id);
	if (codec && codec->name) snprintf(out_props->codec_name, sizeof(out_props->codec_name), "%s", codec->name);
	else snprintf(out_props->codec_name, sizeof(out_props->codec_name), "unknown");
	if (format_ctx->duration!=AV_NOPTS_VALUE)	out_props->duration_seconds=(double)format_ctx->duration/AV_TIME_BASE;
	if (stream->nb_frames>0) out_props->frame_count=stream->nb_frames;
	else if (stream->avg_frame_rate.den>0 && out_props->duration_seconds>0){
		double fps=av_q2d(stream->avg_frame_rate);
		out_props->frame_count=(long long)(fps *out_props->duration_seconds);
	}
	avformat_close_input(&format_ctx);
	return 0;
}
