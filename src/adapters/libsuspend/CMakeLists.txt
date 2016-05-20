add_library(
  suspend STATIC

  libsuspend.c
  sysfs.c
  autosleep.c
  earlysuspend.c
  legacy.c
  mocksuspend.c
)
