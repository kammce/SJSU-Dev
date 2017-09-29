To use the FreeRTOS trace:
    - Change "#define configUSE_TRACE_FACILITY" to a 1 at FreeRTOSConfig.h
    - Define the buffer size as desired at trcConfig.h "#define EVENT_BUFFER_SIZE 1000"
        *  1000 will roughly consume 7K of RAM, so play with it a little
        *  2000 uses roughly 11K of RAM, and 5000 uses roughly 22K of RAM
        *  The larger the size, the larger recording buffer
        *  You can choose ring buffer, or stop buffer at trcConfig.h

Warnings:
    - Trace recorder is not known to work correctly until started by an RTOS task
        *  For example, it did not work if we started it at main.cpp

Instructions:
    - The code to start the trace is:
        vTraceClear();
        printf("Recorder started: %s\n", uiTraceStart() ? "OK" : "ERROR");
    - The code to save the trace is:
        vTraceStop();
        void * buffer = vTraceGetTraceBuffer();
        size_t buffer_size = uiTraceGetTraceBufferSize();

        const bool ok = (FR_OK == Storage::write("1:trcf3.bin", buffer, buffer_size));
        printf("%s: %u bytes from %p\n", ok ? "Wrote" : "Failed to write ", buffer_size, buffer);
    - The code above writes the trace at the SD card, then open the file in TraceAlyzer Windows program