#pragma once

// Глобальні змінні для стрічок
extern Adafruit_NeoPixel* strip_main;
extern Adafruit_NeoPixel* strip_bg;
extern Adafruit_NeoPixel* strip_service;

// Глобальні змінні для стану стрічок
extern bool strip_main_initialized;
extern bool strip_bg_initialized;
extern bool strip_service_initialized;

// Глобальний м'ютекс для стрічок
extern SemaphoreHandle_t stripMutex;

// Індекс LED домашнього району
extern uint8_t homeDistrict; 