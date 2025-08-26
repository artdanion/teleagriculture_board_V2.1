#pragma once

#include <Arduino.h>
#include <vector>
#include <init_Board.h>
#include <board_credentials.h>

// Web functions
void handleRoot();
void handleNotFound();
void drawGraph();
String measurementsTable();
String connectorTable();
