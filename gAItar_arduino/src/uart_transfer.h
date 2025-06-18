#ifndef UART_TRANSFER_H
#define UART_TRANSFER_H

#include <Arduino.h>

/**
 * UART Communication System for Guitar Control Interface
 * Handles file transfer, command processing, and SD card file management
 * Implements binary protocol for reliable data transmission over UART
 */

/**
 * Lists all files on SD card to Serial output for debugging
 * Recursively traverses directory structure and displays file sizes
 */
void listFilesOnSD();

/**
 * Lists all files on SD card over specified UART interface
 * Used for remote file system browsing via UART commands
 * 
 * @param uart UART interface for file list transmission
 */
void listFilesOnSDUart(Uart& uart);

/**
 * Main instruction receiver for real-time command processing
 * Handles binary protocol messages for play, pause, and list commands
 * Implements state machine for reliable packet parsing
 * 
 * @param instrUart UART interface for incoming command messages
 */
void instructionReceiverRTOS(Uart &instrUart);

/**
 * File search function for locating songs by metadata
 * Constructs file paths using genre/artist/title hierarchy
 * Returns full path to binary song file if found
 * 
 * @param title Song title for file search
 * @param artist Artist name for directory navigation
 * @param genre Genre category for top-level directory
 * @return Full file path if found, nullptr if not found
 */
const char* findFileSimple(const char* title, const char* artist, const char* genre);

/**
 * Creates directory structure recursively for file storage
 * Thread-safe implementation with semaphore protection for SD access
 * Used during file transfer operations to ensure directory hierarchy exists
 * 
 * @param fullPath Complete file path including directory structure
 * @return true if directories created successfully, false on failure
 */
bool createDirectoriesRTOS_static(const char* fullPath);

/**
 * Binary file receiver for transferring song files over UART
 * Implements chunked transfer protocol with acknowledgment and retry logic
 * Handles file creation, directory structure, and error recovery
 * 
 * @param fileUart UART interface for file data reception
 */
void fileReceiverRTOS_char(Uart &fileUart);

#endif // UART_TRANSFER_H