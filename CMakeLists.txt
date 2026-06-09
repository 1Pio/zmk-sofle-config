if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources_ifdef(CONFIG_ZMK_RGB_UNDERGLOW app PRIVATE src/sofle_layer_rgb.c)
endif()
