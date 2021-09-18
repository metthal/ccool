#pragma once

namespace ccool {

/**
 * Anything after calling this function will run in daemonized context.
 * No terminal will be attached to the process and it will run in its own session.
 * This function calls fork() and exit() several times so this function
 * should be called as soon as possible and it shouldn't own any resources
 * which require any kind of manual release.
 */
void daemonize();

} // namespace ccool
