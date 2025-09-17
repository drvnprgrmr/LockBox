#pragma once
#include <driver/gpio.h>
#include <stdint.h>

/**
 * @brief Logic level of a key in the matrix
 */
enum class KeyLevel {
    LOW,   ///< Key line is low
    HIGH   ///< Key line is high
};

/**
 * @brief State of the key based on transitions
 */
enum class KeyState {
    IDLE,      ///< Key is low and inactive
    RELEASED,  ///< Key was active but now low
    PRESSED,   ///< Key is high after being idle
    HELD       ///< Key is high for longer than hold time
};

/**
 * @brief Representation of a single keypad key
 */
struct Key {
    char chr;            ///< Character this key represents
    KeyState state;      ///< Current state of the key
    int64_t holdTimer{}; ///< Timestamp for hold detection (µs)
};
