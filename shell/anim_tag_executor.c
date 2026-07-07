/*
compiles with gcc -O2 -Wall animation_analyzer.c anim_tag_executor.c -lavformat -lavcodec -lavutil -o visual_inspector
this has no equivalent to tag_extractor's detailscan02.sh yet...
*/

#include <stdio.h>
#include <stdlib.h>
#include "animation_analyzer.h"

int main(int argc, char *argv[])
{
	if (argc<2){printf("Error: Missing target file path.\n"); return 1;}
	struct AnimationProperties props;
	if (extract_animation_properties(argv[1], &props)!=0) return 1;
	printf("%d\t%d\t%lld\t%.4f\t%s\t%d\t%d\n", props.width, props.height, props.frame_count, props.duration_seconds, props.codec_name, props.video_stream_count, props.audio_stream_count);
	return 0;
}
