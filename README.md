# koalaStreamer
##ffmpeg
`diff --git a/libavcodec/audiotoolboxenc.c b/libavcodec/audiotoolboxenc.c
index c47fbd1b2d..98558316fc 100644
--- a/libavcodec/audiotoolboxenc.c
+++ b/libavcodec/audiotoolboxenc.c
@@ -615,7 +615,7 @@ static const AVOption options[] = {
         .capabilities   = AV_CODEC_CAP_DR1 | AV_CODEC_CAP_DELAY __VA_ARGS__, \
         .sample_fmts    = (const enum AVSampleFormat[]) { \
             AV_SAMPLE_FMT_S16, \
-            AV_SAMPLE_FMT_U8,  AV_SAMPLE_FMT_NONE \
+            AV_SAMPLE_FMT_U8,AV_SAMPLE_FMT_FLT ,AV_SAMPLE_FMT_NONE \
         }, \
         .caps_internal  = FF_CODEC_CAP_INIT_THREADSAFE, \
         .profiles       = PROFILES, \`
         