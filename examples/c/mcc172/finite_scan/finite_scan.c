/*****************************************************************************

    MCC 172 Functions Demonstrated:
        mcc172_a_in_scan_start
        mcc172_a_in_scan_read

    Purpose:
        Perform a finite acquisition on 1 or more channels.

    Description:
        Acquires blocks of analog input data for a user-specified group
        of channels.  The last sample of data for each channel is
        displayed for each block of data received from the device.  The
        acquisition is stopped when the specified number of samples is
        acquired for each channel.

*****************************************************************************/
#include "../../daqhats_utils.h"

int main(void)
{
    int result = RESULT_SUCCESS;
    uint8_t address = 0;
    char channel_string[512];
    char options_str[512];
    char c;
    char display_header[512];
    int i;

    // Set the channel mask which is used by the library function
    // mcc172_a_in_scan_start to specify the channels to acquire.
    // The functions below, will parse the channel mask into a
    // character string for display purposes.
    uint8_t channel_mask = {CHAN0 | CHAN1};
    convert_chan_mask_to_string(channel_mask, channel_string);

    int max_channel_array_length = mcc172_info()->NUM_AI_CHANNELS;
    int channel_array[max_channel_array_length];
    uint8_t num_channels = convert_chan_mask_to_array(channel_mask,
        channel_array);

    uint32_t samples_per_channel = 100000;
    uint32_t buffer_size = samples_per_channel * num_channels;
    double read_buf[buffer_size];
    int total_samples_read = 0;

    int32_t read_request_size = 500;
    double timeout = 5.0;

    double scan_rate = 51200.0;
    double actual_scan_rate = 0.0;

    uint32_t options = OPTS_DEFAULT;

    uint16_t read_status = 0;
    uint32_t samples_read_per_channel = 0;
    uint8_t synced;
    uint8_t clock_source;
    uint8_t iepe_enable;

    /*
    // Select an MCC172 HAT device to use.
    if (select_hat_device(HAT_ID_MCC_172, &address))
    {
        // Error getting device.
        return -1;
    }
    */
    address = 3;
    
    printf ("\nSelected MCC 172 device at address %d\n", address);

    // Open a connection to the device.
    result = mcc172_open(address);
    STOP_ON_ERROR(result);

    // Turn on IEPE supply?
    printf("Enable IEPE power [y or n]?  ");
    scanf("%c", &c);
    if ((c == 'y') || (c == 'Y'))
    {
        iepe_enable = 1;
    }
    else if ((c == 'n') || (c == 'N'))
    {
        iepe_enable = 0;
    }
    else
    {
        printf("Error: Invalid selection\n");
        mcc172_close(address);
        return 1;
    }
    
    for (i = 0; i < num_channels; i++)
    {
        result = mcc172_IEPE_config_write(address, channel_array[i], 
            iepe_enable);
        STOP_ON_ERROR(result);
    }
    
    // Set the ADC clock to the desired rate.
    result = mcc172_a_in_clock_config_write(address, 0, scan_rate);
    STOP_ON_ERROR(result);
    printf("clock write\n");
    
    // Wait for the ADCs to synchronize.
    do
    {
        result = mcc172_a_in_clock_config_read(address, &clock_source,
            &actual_scan_rate, &synced);
        STOP_ON_ERROR(result);
        printf("clock read\n");
        usleep(5000);
    } while (synced == 0);

    convert_options_to_string(options, options_str);

    printf("\nMCC 172 finite scan example\n");
    printf("    Functions demonstrated:\n");
    printf("        mcc172_a_in_scan_start\n");
    printf("        mcc172_a_in_scan_read\n");
    printf("    IEPE power: %s\n", iepe_enable ? "on" : "off");
    printf("    Channels: %s\n", channel_string);
    printf("    Samples per channel: %d\n", samples_per_channel);
    printf("    Requested scan rate: %-10.2f\n", scan_rate);
    printf("    Actual scan rate: %-10.2f\n", actual_scan_rate);
    printf("    Options: %s\n", options_str);

    printf("\nPress ENTER to continue\n");
    scanf("%c", &c);

    // Configure and start the scan.
    result = mcc172_a_in_scan_start(address, channel_mask, samples_per_channel,
        options);
    STOP_ON_ERROR(result);

    printf("Starting scan ... Press ENTER to stop\n\n");

    // Create the header containing the column names.
    strcpy(display_header, "Samples Read    Scan Count    ");
    for (i = 0; i < num_channels; i++)
    {
        sprintf(channel_string, "Channel %d   ", channel_array[i]);
        strcat(display_header, channel_string);
    }
    strcat(display_header, "\n");
    printf("%s", display_header);

    // Continuously update the display value until enter key is pressed
    // or the number of samples requested has been read.
    do
    {
        // Read the specified number of samples.
        result = mcc172_a_in_scan_read(address, &read_status, read_request_size,
            timeout, read_buf, buffer_size, &samples_read_per_channel);
        STOP_ON_ERROR(result);
        if (read_status & STATUS_HW_OVERRUN)
        {
            printf("\n\nHardware overrun\n");
            break;
        }
        else if (read_status & STATUS_BUFFER_OVERRUN)
        {
            printf("\n\nBuffer overrun\n");
            break;
        }

        total_samples_read += samples_read_per_channel;

        // Display the last sample for each channel.
        printf("\r%12.0d    %10.0d ", samples_read_per_channel,
            total_samples_read);
        if (samples_read_per_channel > 0)
        {
            int index = samples_read_per_channel * num_channels - num_channels;

            for (i = 0; i < num_channels; i++)
            {
                printf("%10.5f V", read_buf[index + i]);
            }
            fflush(stdout);
        }
    }
    while ((result == RESULT_SUCCESS) && 
           ((read_status & STATUS_RUNNING) == STATUS_RUNNING) && 
           !enter_press());

    printf ("\n");

stop:
    print_error(mcc172_a_in_scan_stop(address));
    print_error(mcc172_a_in_scan_cleanup(address));
    print_error(mcc172_close(address));

    return 0;
}
