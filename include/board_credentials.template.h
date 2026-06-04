#pragma once

#include <Arduino.h>

#ifndef CUSTOM_LORA_FQZ
#define CUSTOM_LORA_FQZ "EU 868 MHz"
#endif

// =============================================================================
//  TEMPLATE for board_credentials.h  (board_credentials.h is git-ignored)
//
//  TWO WAYS to create your board_credentials.h from this file:
//
//  A) Automatic (needs Boards_List.csv, not public):
//        python flash_board.py <KIT_ID>
//     The script fills the __PLACEHOLDERS__ below and builds + uploads.
//
//  B) Manual (no CSV needed - for external devs):
//     1. Copy this file:  include/board_credentials.template.h
//                     ->  include/board_credentials.h
//     2. Replace every __PLACEHOLDER__ below with your own values.
//     3. Build & flash, picking your LoRa region as the env:
//           pio run -e EU -t upload      (or US / AU / AS / JP / KR / IN)
//
//  Example values (format reference):
//     boardID       1014
//     API_KEY       "fkYauyaxirkYjSKrzR8lzKuVnak4l2nV"   (32 chars)
//     OTAA_APPEUI   "0000000000000000"                   (16 hex, usually zeros)
//     OTAA_DEVEUI   "70B3D57ED005D967"                   (16 hex)
//     OTAA_APPKEY   "EDA96CD96FC5A6C31CCC77D65284C6A4"   (32 hex)
//     If you don't use LoRa, leave the OTAA_* placeholders as zeros.
// =============================================================================

inline int boardID = __BOARD_ID__;                  // e.g. 1014
inline String version = "Firmware Version " TAC_VERSION;
inline String API_KEY = "__API_KEY__";              // 32-char API key from the TAC app
inline String lora_fqz = CUSTOM_LORA_FQZ;
inline String OTAA_APPEUI = "__OTAA_APPEUI__";      // 16 hex digits (zeros if unused)
inline String OTAA_DEVEUI = "__OTAA_DEVEUI__";      // 16 hex digits
inline String OTAA_APPKEY = "__OTAA_APPKEY__";      // 32 hex digits
inline String apn = "0000";
inline String gprs_user = "XXXXX";
inline String gprs_pass = "XXXXX";
inline String mqtt_server_ip = "__MQTT_SERVER_IP__"; // default: 158.255.212.248
inline String mqtt_topic = "/TAC/";
inline int mqtt_port = 1883;
inline String osc_ip = "192.168.242.120";
inline int osc_port = 5000;
inline String osc_topic = "/";
inline bool instant_upload = false;

#define LMIC_PRINTF_TO Serial

inline const char *kits_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n"
    "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n"
    "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n"
    "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n"
    "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n"
    "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n"
    "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n"
    "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n"
    "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n"
    "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n"
    "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n"
    "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n"
    "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n"
    "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n"
    "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n"
    "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n"
    "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n"
    "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n"
    "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n"
    "-----END CERTIFICATE-----\n";
