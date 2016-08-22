// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <AzureIoTHub.h>
#include "sdk/schemaserializer.h"

static const char* deviceId = "<DEVICEID>";

static const char* deviceConnectionString = "<CONNECTIONSTRING>";

// Define the Model
BEGIN_NAMESPACE(Contoso);

DECLARE_MODEL(Blinkstat,

    /* Event data (temperature, external temperature and humidity) */
    WITH_DATA(int, Interval),
    WITH_DATA(int, Duration),

    /* Device Info - This is command metadata + some extra fields */
    WITH_DATA(ascii_char_ptr, DeviceId),
    WITH_DATA(_Bool, IsSimulatedDevice),
    
    /* Commands implemented by the device */
    WITH_ACTION(SetInterval, int, interval),
    WITH_ACTION(SetDuration, int, duration)
);

END_NAMESPACE(Contoso);

DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES)

EXECUTE_COMMAND_RESULT SetInterval(Blinkstat* blinkstat, int interval)
{
    LogInfo("Received interval %d\r\n", interval);
    blinkstat->Interval = interval;
    return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT SetDuration(Blinkstat* blinkstat, int duration)
{
    LogInfo("Received duration %d\r\n", duration);
    blinkstat->Duration = duration;
    return EXECUTE_COMMAND_SUCCESS;
}

void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    int messageTrackingId = (intptr_t)userContextCallback;
    LogInfo("Sent Message Id: %d.\r\n", messageTrackingId);
    LogInfo("sendCallback called. Result is: %s \r\n", ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size)
{
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
    static unsigned int messageTrackingId;

    if (messageHandle == NULL)
    {
        LogInfo("unable to create a new IoTHubMessage\r\n");
    }
    else
    {
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)(uintptr_t)messageTrackingId) != IOTHUB_CLIENT_OK)
        {
            LogInfo("failed to hand over the message to IoTHubClient");
        }
        else
        {
            LogInfo("IoTHubClient accepted the message for delivery\r\n");
        }

        IoTHubMessage_Destroy(messageHandle);
    }
    free((void*)buffer);
    messageTrackingId++;
}

/*this function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT receiveCloudToDeviceMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    LogInfo("Enter IotHubMessage\r\n");
    IOTHUBMESSAGE_DISPOSITION_RESULT result = IOTHUBMESSAGE_REJECTED;
    Blinkstat* blinkstat = (Blinkstat*)userContextCallback;

    // Retrieve properties from the message
    MAP_HANDLE mapProperties = IoTHubMessage_Properties(message);
    if (mapProperties != NULL)
    {
        result = IOTHUBMESSAGE_ACCEPTED;
        const char*const* keys;
        const char*const* values;
        size_t propertyCount = 0;
        if (Map_GetInternals(mapProperties, &keys, &values, &propertyCount) == MAP_OK)
        {
            if (propertyCount > 0)
            {
                LogInfo("Message Properties:\r\n");
                for (size_t index = 0; index < propertyCount; index++)
                {
                    if(!strcmp(keys[index],"interval")) SetInterval(blinkstat, atoi(values[index]));
                    if(!strcmp(keys[index],"duration")) SetDuration(blinkstat, atoi(values[index]));
                    LogInfo("\tKey: %s Value: %s\r\n", keys[index], values[index]);
                }
                LogInfo("\r\n");
            }
        }
        
    }
    else
    {
        LogInfo("IOTHUBMESSAGE_REJECTED\r\n");
    }
    return result;
}

void azureiothubsample_run(void)
{
        srand((unsigned int)time(NULL));
        if (serializer_init(NULL) != SERIALIZER_OK)
        {
            LogInfo("Failed on serializer_init\r\n");
        }
        else
        {
            IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
            iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(deviceConnectionString, HTTP_Protocol);
            if (iotHubClientHandle == NULL)
            {
                LogInfo("Failed on IoTHubClient_CreateFromConnectionString\r\n");
            }
            else
            {
#ifdef MBED_BUILD_TIMESTAMP
                // For mbed add the certificate information
                if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
                {
                    LogInfo("failure to set option \"TrustedCerts\"\r\n");
                }
#endif // MBED_BUILD_TIMESTAMP
                unsigned int minimumPollingTime = 0;
                if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK)
                {
                    LogInfo("failure to set option \"MinimumPollingTime\"\r\n");
                }

                Blinkstat* blinkstat = CREATE_MODEL_INSTANCE(Contoso, Blinkstat);
                if (blinkstat == NULL)
                {
                    LogInfo("Failed on CREATE_MODEL_INSTANCE\r\n");
                }
                else
                {
                    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveCloudToDeviceMessage, blinkstat) != IOTHUB_CLIENT_OK)
                    {
                        LogInfo("unable to IoTHubClient_SetMessageCallback\r\n");
                    }
                    else
                    {

                        /* send the device info upon startup so that the cloud app knows
                        what commands are available and the fact that the device is up */
                        blinkstat->DeviceId = (char*)deviceId;
                        blinkstat->IsSimulatedDevice = false;
                        unsigned char* buffer;
                        size_t bufferSize;
                        /* Here is the actual send of the Device Info */
                        if (SERIALIZE(&buffer, &bufferSize, blinkstat->DeviceId, blinkstat->IsSimulatedDevice) != IOT_AGENT_OK)
                        {
                            LogInfo("Failed serializing\r\n");
                        }
                        blinkstat->Interval = 999;
                        blinkstat->Duration = 999;
                        int sendCycle = 30;
                        int currentCycle = 0;
                        while (currentCycle < sendCycle)
                        {
                            unsigned char*tempbuffer;
                            size_t tempbufferSize;
                            if (SERIALIZE(&tempbuffer, &tempbufferSize, blinkstat->Interval, blinkstat->Duration) != IOT_AGENT_OK)
                            {
                                LogInfo("Failed sending sensor value\r\n");
                            }
                            else
                            {
                                sendMessage(iotHubClientHandle, tempbuffer, tempbufferSize);
                            }
                            currentCycle++;
                            IoTHubClient_LL_DoWork(iotHubClientHandle);
                            ThreadAPI_Sleep(1000);
                        }

                    }
                    DESTROY_MODEL_INSTANCE(blinkstat);
                }
                IoTHubClient_LL_Destroy(iotHubClientHandle);
            }
            serializer_deinit();
    }
}
