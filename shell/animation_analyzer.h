#ifndef ANIMATION_ANALYZER_H
#define ANIMATION_ANALYZER_H

struct AnimationProperties{
	int width;
	int height;
	long long frame_count;
	double duration_seconds;
	char codec_name[64];
	int video_stream_count;   // Tracks if a file has multiple video tracks
	int audio_stream_count;   // Critical for filtering out AV/movie material
};

int extract_animation_properties(const char *filepath, struct AnimationProperties *out_props);

#endif
