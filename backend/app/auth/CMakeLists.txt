cmake_minimum_required(VERSION 3.12.4)

# `models` for user.h -- it used to arrive through cwfr::framework's interface
# back when the application archives were baked into it.
cwfr_add_lib(auth LINK_LIBS models cwfr::framework)
