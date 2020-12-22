/**
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "log.h"
#include "agent.h"
#include "thread.h" // for mutexes

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef NO_ATOMICS
#include <stdatomic.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif

#define BUFFER_SIZE 4096

struct juice_logger {
	mutex_t log_mutex;
	volatile juice_log_cb_t log_cb;
	void *user_ptr;
#ifdef NO_ATOMICS
	volatile juice_log_level_t log_level;
#else
	_Atomic(juice_log_level_t) log_level;
#endif
};

static const char *log_level_names[] = {"VERBOSE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

static const char *log_level_colors[] = {
    "\x1B[90m",        // grey
    "\x1B[96m",        // cyan
    "\x1B[39m",        // default foreground
    "\x1B[93m",        // yellow
    "\x1B[91m",        // red
    "\x1B[97m\x1B[41m" // white on red
};

static bool use_color(void) {
#ifdef _WIN32
	return false;
#else
	return isatty(fileno(stdout)) != 0;
#endif
}

juice_logger_t *juice_logger_create(const juice_log_config_t *config) {
	juice_logger_t *logger = calloc(1, sizeof(juice_logger_t));

	if (logger == NULL) {
		return NULL;
	}

	mutex_init(&logger->log_mutex, 0);
	logger->log_cb = config->log_cb;
	logger->user_ptr = config->user_ptr;
#ifdef NO_ATOMICS
	logger->log_level = JUICE_LOG_LEVEL_WARN;
#else
	atomic_init(&logger->log_level, JUICE_LOG_LEVEL_WARN);
#endif

	return logger;
}

void juice_logger_destroy(juice_logger_t *logger) {
	mutex_destroy(&logger->log_mutex);
	free(logger);
}

void juice_logger_set_log_level(juice_logger_t *logger, juice_log_level_t level) {
#ifdef NO_ATOMICS
	mutex_lock(&logger->log_mutex);
	logger->log_level = level;
	mutex_unlock(&logger->log_mutex);
#else
	atomic_store(&logger->log_level, level);
#endif
}

void juice_log_write(juice_logger_t *logger, juice_log_level_t level, const char *file, int line,
                     const char *fmt, ...) {
#ifdef NO_ATOMICS
	mutex_lock(&logger->log_mutex);
	if (level < logger->log_level) {
		mutex_unlock(&logger->log_mutex);
		return;
	}
#else
	if (level < atomic_load(&logger->log_level) || level == JUICE_LOG_LEVEL_NONE)
		return;

	mutex_lock(&logger->log_mutex);
#endif

	const char *filename = file + strlen(file);
	while (filename != file && *filename != '/' && *filename != '\\')
		--filename;
	if (filename != file)
		++filename;

	if (logger->log_cb) {
		char message[BUFFER_SIZE];
		int len = snprintf(message, BUFFER_SIZE, "%s:%d: ", filename, line);
		len = len >= 0 ? len : 0;

		va_list args;
		va_start(args, fmt);
		len = vsnprintf(message + len, BUFFER_SIZE - len, fmt, args);
		va_end(args);

		if (len >= 0)
			logger->log_cb(level, message, logger->user_ptr);
	} else {
		time_t t = time(NULL);
		struct tm *lt = localtime(&t);
		char buffer[16];
		if (strftime(buffer, 16, "%H:%M:%S", lt) == 0)
			buffer[0] = '\0';

		if (use_color())
			fprintf(stdout, "%s", log_level_colors[level]);

		fprintf(stdout, "%s %-7s %s:%d: ", buffer, log_level_names[level], filename, line);

		va_list args;
		va_start(args, fmt);
		vfprintf(stdout, fmt, args);
		va_end(args);

		if (use_color())
			fprintf(stdout, "%s", "\x1B[0m\x1B[0K");

		fprintf(stdout, "\n");
		fflush(stdout);
	}
	mutex_unlock(&logger->log_mutex);
}
