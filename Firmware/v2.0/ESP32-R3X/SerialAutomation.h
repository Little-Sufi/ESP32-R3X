#pragma once
#include <Arduino.h>

void serialAutomationInit();
void serialAutomationPoll();

void cliPrint(const String& s);
void cliPrintln(const String& s = "");
void cliPrintf(const char* format, ...);

// Virtual buttons for automation injection
bool isSerialButtonPressed(int buttonPin);
bool isSerialButtonPressedEdge(int buttonPin);
bool isSerialExitRequested();
void serialAutomationClearExit();

// Control functions
void serialAutomationSimulateKey(int buttonPin, uint32_t holdDurationMs = 200);
void serialAutomationRequestExit();

// System diagnostics & testing
void serialAutomationRunDiag();
void serialAutomationRunTest(const String& target);
void serialAutomationDumpHeap();
void serialAutomationDumpStatus();

// Direct tool launch hook
typedef void (*SerialLaunchCallback)(int menu_idx, int sub_idx, int layer);
void serialAutomationSetLaunchCallback(SerialLaunchCallback cb);
