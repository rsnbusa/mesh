#define GLOBAL

#include "globals.h"
#include "includes.h"
extern void ssdString(int x, int y, char * que,bool centerf);
extern void delay(uint32_t cuanto);

void task_fatal_error(void)
{
#ifdef DISPLAY
    u8g2_ClearBuffer(&u8g2);
    ssdString(10,38,(char*)"OTA FAIL",true);
    delay(3000);
    u8g2_ClearBuffer(&u8g2);
	u8g2_SendBuffer(&u8g2);
#endif
}

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}


void ota_task(void *pvParameter)
{

#ifdef DISPLAY
    u8g2_ClearBuffer(&u8g2);
    ssdString(10,38,(char*)"OTA",true);
#endif
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    char  *ota_write_data;
    ota_write_data=(char*)malloc(1030);
    if(ota_write_data)
        bzero(ota_write_data,1030);

    printf("Starting OTA example\n");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
       printf("Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x\n",
                 configured->address, running->address);
       printf("(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)\n");
    }

    printf("Running partition type %d subtype %d (offset 0x%08x)\n",
             running->type, running->subtype, running->address);

    esp_http_client_config_t config = {
        .url = OTAURL,
        .cert_pem =NULL,
        .timeout_ms = OTADEL,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        printf("Failed to initialise HTTP connection\n");
        task_fatal_error();
        vTaskDelete(NULL);
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        printf("Failed to open HTTP connection: %s\n", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
        vTaskDelete(NULL);
    }
    esp_http_client_fetch_headers(client);

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    printf( "Writing to partition subtype %d at offset 0x%x\n",update_partition->subtype, update_partition->address);

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, 1024);
        if (data_read < 0) {
            printf("Error: SSL data read error\n");
            http_cleanup(client);
            task_fatal_error();
            vTaskDelete(NULL);
        } else if (data_read > 0) {
            if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    printf("New firmware version: %s\n", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        printf( "Running firmware version: %s\n", running_app_info.version);
                    }

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        printf("Last invalid firmware version: %s\n", invalid_app_info.version);
                    }

                    // check current version with last invalid partition
                    if (last_invalid_app != NULL) {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                            printf("New version is the same as invalid version.\n");
                            printf( "Previously, there was an attempt to launch the firmware with %s version, but it failed.\n", invalid_app_info.version);
                            printf( "The firmware has been rolled back to the previous version.\n");
                            http_cleanup(client);
                            task_fatal_error();
                            vTaskDelete(NULL);
                        }
                    }

                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) <= 0) {
                        printf("Current running version is less than or equal. We will not continue the update.\n");
                        http_cleanup(client);
                        task_fatal_error();
                        vTaskDelete(NULL);
                    }

                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
                    if (err != ESP_OK) {
                        printf("esp_ota_begin failed (%s)\n", esp_err_to_name(err));
                        http_cleanup(client);
                        esp_ota_abort(update_handle);
                        task_fatal_error();
                        vTaskDelete(NULL);
                    }
#ifdef DISPLAY
                    u8g2_ClearBuffer(&u8g2);
                    ssdString(10,38,(char*)"OTA ST",true);
#endif
                     printf("esp_ota_begin succeeded\n");
                } else {
                    printf( "received package is not fit len\n");
                    http_cleanup(client);
                    esp_ota_abort(update_handle);
                    task_fatal_error();
                    vTaskDelete(NULL);                    
                }
            }
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                http_cleanup(client);
                esp_ota_abort(update_handle);
                task_fatal_error();
                vTaskDelete(NULL);                
            }
            binary_file_length += data_read;
            printf(".");
            // printf("Written image length %d\n", binary_file_length);
        } else if (data_read == 0) {
           /*
            * As esp_http_client_read never returns negative error code, we rely on
            * `errno` to check for underlying transport connectivity closure if any
            */
            if (errno == ECONNRESET || errno == ENOTCONN) {
                printf("Connection closed, errno = %d\n", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true) {
                printf( "Connection closed\n");
                break;
            }
        }
    }
    printf("Total Write binary data length: %d\n", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) {
       printf( "Error in receiving complete file\n");
        http_cleanup(client);
        esp_ota_abort(update_handle);
        task_fatal_error();
        vTaskDelete(NULL);
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
           printf("Image validation failed, image is corrupted\n");
        } else {
            printf("esp_ota_end failed (%s)!\n", esp_err_to_name(err));
        }
        http_cleanup(client);
        task_fatal_error();
        vTaskDelete(NULL);
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
       printf( "esp_ota_set_boot_partition failed (%s)!\n", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
        vTaskDelete(NULL);
    }
    printf( "Prepare to restart system!\n");
#ifdef DISPLAY
    u8g2_ClearBuffer(&u8g2);
    ssdString(10,38,(char*)"OTA OK",true);
    delay(2000);
#endif
    esp_restart();
    return ;
}

void start_ota()
{
    xTaskCreate(&ota_task,"ota",10000,NULL, 5, NULL);           //start the Virtual Machine connected or not to wifi

}