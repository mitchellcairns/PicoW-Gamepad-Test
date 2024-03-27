#include "main.h"

static btstack_packet_callback_registration_t hci_event_callback_registration;
static uint8_t hid_service_buffer[700];
static uint8_t pnp_service_buffer[200];
static const char hid_device_name[] = "Wireless Gamepad";

bool interval_run(uint32_t timestamp, uint32_t interval, interval_s *state)
{
  state->this_time = timestamp;

  // Clear variable
  uint32_t diff = 0;

  // Handle edge case where time has
  // looped around and is now less
  if (state->this_time < state->last_time)
  {
    diff = (0xFFFFFFFF - state->last_time) + state->this_time;
  }
  else if (state->this_time > state->last_time)
  {
    diff = state->this_time - state->last_time;
  }
  else
    return false;

  // We want a target rate according to our variable
  if (diff >= interval)
  {
    // Set the last time
    state->last_time = state->this_time;
    return true;
  }
  return false;
}

void init_bt()
{
    btstack_memory_init();

    bd_addr_t newAddr = {0x7c,
                         0xbb,
                         0x8a,
                         (uint8_t)(get_rand_32() % 0xff),
                         (uint8_t)(get_rand_32() % 0xff),
                         (uint8_t)(get_rand_32() % 0xff)};

    if (cyw43_arch_init())
    {
        return;
    }

    gap_discoverable_control(1);
    gap_set_class_of_device(0x2508);
    gap_set_local_name("XInput Controller");
    gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_ROLE_SWITCH |
                                         LM_LINK_POLICY_ENABLE_SNIFF_MODE);
    gap_set_allow_role_switch(true);

    hci_set_chipset(btstack_chipset_cyw43_instance());
    hci_set_bd_addr(newAddr);

    // L2CAP
    l2cap_init();
    sm_init();
    // SDP Server
    sdp_init();

    hid_sdp_record_t hid_sdp_record = {0x2508,
                                       33,
                                       1,
                                       1,
                                       1,
                                       0,
                                       0,
                                       0xFFFF,
                                       0xFFFF,
                                       3200,
                                       xinput_hid_report_descriptor,
                                       XINPUT_HID_REPORT_MAP_LEN,
                                       hid_device_name};

    // Register SDP services
    memset(hid_service_buffer, 0, sizeof(hid_service_buffer));
    create_sdp_hid_record(hid_service_buffer, &hid_sdp_record);
    sdp_register_service(hid_service_buffer);

    memset(pnp_service_buffer, 0, sizeof(pnp_service_buffer));
    create_sdp_pnp_record(pnp_service_buffer, DEVICE_ID_VENDOR_ID_SOURCE_USB,
                          0x045E, 0x0B13, 0x0001);
    sdp_register_service(pnp_service_buffer);

    // HID Device
    hid_device_init(1, XINPUT_HID_REPORT_MAP_LEN,
                    xinput_hid_report_descriptor);
}

volatile uint16_t cid = 0;

uint32_t _report_owner_0;
uint32_t _report_owner_1;
auto_init_mutex(_report_mutex);
volatile bool _report_ready = false;

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t packet_size)
{
    uint8_t status;
    if (packet_type != HCI_EVENT_PACKET)
    {
        return;
    }
    switch (packet[0])
    {
    case HCI_EVENT_HID_META:
        switch (hci_event_hid_meta_get_subevent_code(packet))
        {
        case HID_SUBEVENT_CONNECTION_OPENED:
            status = hid_subevent_connection_opened_get_status(packet);
            if (status)
            {
                // outgoing connection failed
                cid = 0;
                return;
            }
            cid = hid_subevent_connection_opened_get_hid_cid(packet);
            hid_device_request_can_send_now_event(cid);
            break;
        case HID_SUBEVENT_CONNECTION_CLOSED:
            cid = 0;
            break;
        case HID_SUBEVENT_GET_PROTOCOL_RESPONSE:
            break;
        case HID_SUBEVENT_CAN_SEND_NOW:
            //mutex_enter_blocking(&_report_mutex);
            //_report_ready = true;
            //mutex_exit(&_report_mutex);
            break;
        }
        break;
    }
}

static void hid_report_handler(uint16_t cid, hid_report_type_t report_type, uint16_t report_id, int report_size, uint8_t *report)
{
}

uint32_t _timer_owner_0;
uint32_t _timer_owner_1;
auto_init_mutex(_timer_mutex);

volatile uint32_t _timestamp = 0;

void loop_1()
{
    // Enable lockout victimhood :,)
    multicore_lockout_victim_init();

    static interval_s interval_0 = {0};
    static xi_input_s xinput_data = {.stick_left_x = 0xFFFF/2, .stick_left_y = 0xFFFF/2, .stick_right_x = 0xFFFF/2, .stick_right_y = 0xFFFF/2};
    static uint8_t report_data[64];
    static int flip = 0;

    

    for(;;)
    {
        if (mutex_try_enter(&_timer_mutex, &_timer_owner_0))
        {
        _timestamp = time_us_32();
        mutex_exit(&_timer_mutex);
        }

        if(cid>0)
        {
            if(interval_run(_timestamp, 8000, &interval_0))
            {
                
                if(flip==0)
                {
                    xinput_data.stick_left_x = xinput_data.stick_left_x+10000;
                    flip = 1;
                }
                else if(flip==1)
                {
                    xinput_data.stick_left_x = xinput_data.stick_left_x-10000;
                    flip = 2;
                }
                else if(flip==2)
                {
                    xinput_data.stick_left_x = xinput_data.stick_left_x-10000;
                    flip = 3;
                }
                else if(flip==3)
                {
                    xinput_data.stick_left_x = xinput_data.stick_left_x+10000;
                    flip = 0;
                }

                report_data[0] = 0xA1;
                report_data[1] = 0x01;
                memcpy(&report_data[2], &xinput_data, sizeof(xi_input_s));
                hid_device_send_interrupt_message(cid, report_data, 18);
            }
        }

    }
    
}

void loop_0()
{
    init_bt();

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    hid_device_register_packet_handler(&packet_handler);
    //hid_device_register_report_data_callback(&hid_report_handler);

    // turn on!
    hci_power_control(HCI_POWER_ON);

    btstack_run_loop_execute();
}

int main()
{
    stdio_init_all();

    // Launch second core
    multicore_launch_core1(loop_1);
    loop_0();

    return 0;
}