#include "txt_writer.h"

char* logFile;
char* logDir;

bool createFile(const char* filepath) {
    if (!filepath) {
        fprintf(stderr, "Invalid filepath provided.\n");
        return false;
    }

    // Duplicate the filepath for safe modification
    char* path = strdup(filepath);
    if (!path) {
        perror("Error duplicating filepath");
        return false;
    }

    // Extract the directory path
    char* last_slash = strrchr(path, '/');
    if (last_slash) {
        *last_slash = '\0';  // Terminate at the last slash to isolate the directory path

        // Recursively create the directory path if it doesn't exist
        char* temp_path = strdup(path);
        if (!temp_path) {
            perror("Error duplicating path for directory creation");
            free(path);
            return false;
        }

        for (char* p = temp_path; *p; ++p) {
            if (*p == '/') {
                *p = '\0';  // Temporarily terminate the string to test this subpath
                if (strlen(temp_path) > 0 && mkdir(temp_path, 0755) != 0 && errno != EEXIST) {
                    perror("Error creating directory");
                    free(path);
                    free(temp_path);
                    return false;
                }
                *p = '/';  // Restore the slash
            }
        }

        // Create the final directory if it doesn't exist
        if (mkdir(temp_path, 0755) != 0 && errno != EEXIST) {
            perror("Error creating directory");
            free(path);
            free(temp_path);
            return false;
        }

        free(temp_path);
    }

    free(path);

    // Open the file in write mode
    FILE* file = fopen(filepath, "w");
    if (!file) {
        perror("Error creating file");
        return false;
    }

    fclose(file);  // Immediately close the file to ensure it's created
    return true;
}

bool writeLineToFile(const char* filename, const char* line) {
    FILE* file = fopen(filename, "a");  // Open file in append mode
    if (!file) {
        perror("Error opening file");
        return false;
    }

    fprintf(file, "%s\n", line);  // Write the line followed by a newline
    fclose(file);  // Close the file
    return true;
}

int emptyFile(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return -1;
    }
    fclose(file);
    return 0;
}

bool fileLog(const char* format, ...) {
	// Ensure format and required globals are valid
	if (!format || !logDir || !logFile || strlen(logDir) == 0 || strlen(logFile) == 0) {
		fprintf(stderr, "Invalid input parameters.\n");
		return false;
	}

	// Allocate enough memory for the formatted string
	char* log;
	va_list args;

	// First pass to calculate the required size
	va_start(args, format);
	size_t logSize = vsnprintf(NULL, 0, format, args) + 1; // +1 for null-terminator
	va_end(args);

	log = (char*)malloc(logSize);
	if (!log) {
		fprintf(stderr, "Memory allocation failed for log string.\n");
		return false; // Memory allocation failed
	}

	// Second pass to create the formatted string
	va_start(args, format);
	vsnprintf(log, logSize, format, args);
	va_end(args);

	// Check if logDir ends with '/'
	size_t logDirLen = strlen(logDir);
	bool hasTrailingSlash = (logDir[logDirLen - 1] == '/');
	size_t logLocationSize = logDirLen + strlen(logFile) + (hasTrailingSlash ? 0 : 1) + 1; // +1 for null-terminator
	char* logLocation = (char*)malloc(logLocationSize);
	if (!logLocation) {
		fprintf(stderr, "Memory allocation failed for log location.\n");
		free(log); // Free the formatted string before returning
		return false; // Memory allocation failed
	}

	// Construct logLocation
	strcpy(logLocation, logDir);
	if (!hasTrailingSlash) {
		strcat(logLocation, "/");
	}
	strcat(logLocation, logFile);

	// Write the log to the file
	bool success = writeLineToFile(logLocation, log);

	// Free allocated memory
	free(log);
	free(logLocation);

	return success;
}
