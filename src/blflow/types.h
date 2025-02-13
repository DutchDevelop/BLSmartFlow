#ifndef _TYPES
#define _TYPES

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct PrinterVaraiblesStruct{
        int chamberfan = 0;
        double nozzletemp = 0;
        bool online = false;
        String errorcode = "";
        //Time since
        unsigned long disconnectMQTTms = 0;        
    } PrinterVariables;
    PrinterVariables printerVariables;

    typedef struct GlobalVariablesStruct{
        char SSID[32];
        char APPW[63];
        String FWVersion = "Stable 2025.2.13";
        String Host = "BLFLOW";
        bool started = false;
        int fanSpeed = 0;
    } GlobalVariables;
    GlobalVariables globalVariables;

    typedef struct PrinterConfigStruct
    {
        char printerIP[16];             //BBLP IP Address - used for MQTT reports
        char accessCode[9];             //BBLP Access Code - used for MQTT reports
        char serialNumber[16];          //BBLP Serial Number - used for MQTT reports

        char BSSID[18];                 //Nominated AP to connect to (Useful if multiple accesspoints with same name)
        bool rescanWiFiNetwork = false; //Scans available WiFi networks for strongest signal

        // Debugging
        bool debuging = true;          //Debugging for all interactions through functions
        bool debugingchange = true;     //Default debugging level - to shows onChange
        bool mqttdebug = false;         //Writes each packet from BBLP to the serial log

        bool staticFan = false;  
        int staticFanSpeed = 0;

        std::vector<std::pair<float, int>> fanGraph = { //Basic fan speed profile
            {0, 0},
            {50, 0},
            {180, 50},
            {245, 80},
            {350, 100},
            {400, 100}
        };

    } PrinterConfig;
    PrinterConfig printerConfig;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif