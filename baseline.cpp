
/*******************************************************
 * baseline.cpp
 * 
 * Пример демонстрирует базовый сбор телеметрии через NVML:
 *  - Загрузка (utilization) ядра GPU.
 *  - Использование видеопамяти (free/total).
 * 
 * Перед сборкой:
 *  1. Установите драйверы NVIDIA и CUDA toolkit (или отдельно NVML).
 *  2. Убедитесь, что nvml.h доступен компилятору (обычно в /usr/include).
 * 
 * Сборка (Linux):
 *  g++ baseline.cpp -o baseline -lnvidia-ml
 *******************************************************/

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <csignal>

#include <nvml.h>

// Интервал опроса, мс (можете изменить на своё усмотрение)
static const int SAMPLING_INTERVAL_MS = 500;

// Флаг для безопасного завершения (при нажатии Ctrl+C)
static bool g_Running = true;

// Обработчик сигнала прерывания (Ctrl+C)
void signalHandler(int signum)
{
    std::cout << "\nInterrupt signal (" << signum << ") received. Stopping...\n";
    g_Running = false;
}

int main()
{
    // Устанавливаем обработчик сигнала SIGINT (Ctrl+C)
    std::signal(SIGINT, signalHandler);

    nvmlReturn_t nvmlResult = nvmlInit();
    if (nvmlResult != NVML_SUCCESS)
    {
        std::cerr << "Error initializing NVML: " 
                  << nvmlErrorString(nvmlResult) << std::endl;
        return 1;
    }

    // Узнаём, сколько GPU доступно
    unsigned int deviceCount = 0;
    nvmlResult = nvmlDeviceGetCount(&deviceCount);
    if (nvmlResult != NVML_SUCCESS || deviceCount == 0)
    {
        std::cerr << "Error: No NVIDIA devices found or NVML error: "
                  << nvmlErrorString(nvmlResult) << std::endl;
        nvmlShutdown();
        return 1;
    }

    // Для простоты берём первый GPU (index=0)
    nvmlDevice_t device;
    nvmlResult = nvmlDeviceGetHandleByIndex(0, &device);
    if (nvmlResult != NVML_SUCCESS)
    {
        std::cerr << "Error: Unable to get handle for device 0: "
                  << nvmlErrorString(nvmlResult) << std::endl;
        nvmlShutdown();
        return 1;
    }

    // Получим имя GPU (просто для справки)
    char deviceName[64];
    nvmlResult = nvmlDeviceGetName(device, deviceName, sizeof(deviceName));
    if (nvmlResult != NVML_SUCCESS)
    {
        std::cerr << "Error: Unable to get device name: "
                  << nvmlErrorString(nvmlResult) << std::endl;
    }
    else
    {
        std::cout << "Using GPU: " << deviceName << std::endl;
    }

    // Основной цикл опроса
    while (g_Running)
    {
        // Получаем показатели утилизации (загрузка ядра)
        nvmlUtilization_t utilization;
        nvmlResult = nvmlDeviceGetUtilizationRates(device, &utilization);
        if (nvmlResult != NVML_SUCCESS)
        {
            std::cerr << "Error: Unable to get utilization rates: "
                      << nvmlErrorString(nvmlResult) << std::endl;
        }

        // Получаем информацию о памяти
        nvmlMemory_t memInfo;
        nvmlResult = nvmlDeviceGetMemoryInfo(device, &memInfo);
        if (nvmlResult != NVML_SUCCESS)
        {
            std::cerr << "Error: Unable to get memory info: "
                      << nvmlErrorString(nvmlResult) << std::endl;
        }

        // Вычисляем, сколько процентов занято видеопамяти
        double memUsedMB = (memInfo.used / 1024.0 / 1024.0);
        double memTotalMB = (memInfo.total / 1024.0 / 1024.0);
        double memUsagePercent = (memUsedMB / memTotalMB) * 100.0;

        // Выводим в консоль
        std::cout << "[GPU Util: " << utilization.gpu 
                  << "% | Mem Used: " << memUsedMB << "MB / " 
                  << memTotalMB << "MB (" << memUsagePercent << "%)]"
                  << std::endl;

        // Ждём заданный интервал
        std::this_thread::sleep_for(std::chrono::milliseconds(SAMPLING_INTERVAL_MS));
    }

    // Завершаем работу NVML
    nvmlResult = nvmlShutdown();
    if (nvmlResult != NVML_SUCCESS)
    {
        std::cerr << "Error shutting down NVML: " 
                  << nvmlErrorString(nvmlResult) << std::endl;
        return 1;
    }

    std::cout << "Done. Exiting normally." << std::endl;
    return 0;
}
