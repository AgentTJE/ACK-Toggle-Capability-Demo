// This code was generated by a tool on Wed, 3 Jun 2020 22:07:09 GMT.
// DO NOT EDIT MANUALLY.

// Instruction artifact: 'Alexa.BrightnessController'.

#ifndef __ACK_GENERATED_ALEXA_BRIGHTNESS_CONTROLLER_H__
#define __ACK_GENERATED_ALEXA_BRIGHTNESS_CONTROLLER_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _ack_alexa_brightness_controller_set_brightness_directive_payload_object_member
{
    ack_alexa_brightness_controller_set_brightness_directive_payload_object_member_brightness = 1
}
ack_alexa_brightness_controller_set_brightness_directive_payload_object_member;

typedef enum _ack_alexa_brightness_controller_property_name
{
    ack_alexa_brightness_controller_property_name_brightness = 1
}
ack_alexa_brightness_controller_property_name;

typedef enum _ack_alexa_brightness_controller_directive_name
{
    ack_alexa_brightness_controller_directive_name_set_brightness = 1,
    ack_alexa_brightness_controller_directive_name_adjust_brightness = 2
}
ack_alexa_brightness_controller_directive_name;

typedef enum _ack_alexa_brightness_controller_adjust_brightness_directive_payload_object_member
{
    ack_alexa_brightness_controller_adjust_brightness_directive_payload_object_member_brightness_delta = 1
}
ack_alexa_brightness_controller_adjust_brightness_directive_payload_object_member;

#ifdef __cplusplus
}
#endif

#endif // ndef __ACK_GENERATED_ALEXA_BRIGHTNESS_CONTROLLER_H__
