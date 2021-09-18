#pragma once

#define STRFY_HELPER(x)  x
#define STRFY(x)         STRFY_HELPER(x)
#define LOGGER_NAME      "ccoold"
#define LOG              spdlog::get(STRFY(LOGGER_NAME))
