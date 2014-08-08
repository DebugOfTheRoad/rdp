#include "protocol.h"


bool protocol_set_header(protocol_header* header)
{
    if (header->protocol >= proto_end) {
        return false;
    }
    ui16 psize = 0;
    switch (header->protocol) {
    case proto_ack:
        psize = sizeof(protocol_ack);
        break;
    case   proto_ctrl:
        psize = sizeof(protocol_ctrl);
        break;
    case   proto_ctrl_ack:
        psize = sizeof(protocol_ctrl_ack);
        break;
    case    proto_connect:
        psize = sizeof(protocol_connect);
        break;
    case    proto_connect_ack:
        psize = sizeof(protocol_connect_ack);
        break;
    case    proto_disconnect:
        psize = sizeof(protocol_disconnect);
        break;
    case    proto_heartbeat:
        psize = sizeof(protocol_heartbeat);
        break;
    case     proto_data:
        psize = sizeof(protocol_data);
        break;
    case     proto_data_noack:
        psize = sizeof(protocol_data_noack);
        break;
    case     proto_udp_data:
        psize = sizeof(protocol_udp_data);
        break;
    }

    ui8* c = (ui8*)header;
    ++c;
    --psize;
    ui8 checksum = CHECKSUM_SEED;
    for (ui16 i = 0; i < psize; ++i) {
        checksum ^= *c++;
    }
    header->check_sum = checksum;
    return true;
}
bool protocol_check_header(protocol_header* header, ui16 size)
{
    if (header->protocol >= proto_end) {
        return false;
    }
    ui16 psize = 0;
    switch (header->protocol) {
    case proto_ack: {
        protocol_ack* p = (protocol_ack*)header;
        if (sizeof(protocol_ack)+p->seq_num_ack_count*sizeof(ui32) != size) {
            return false;
        }
    }
    psize = sizeof(protocol_ack);
    break;
    case   proto_ctrl:
        if (sizeof(protocol_ctrl)  != size) {
            return false;
        }
        psize = sizeof(protocol_ctrl);
        break;
    case   proto_ctrl_ack:
        if (sizeof(protocol_ctrl_ack) != size) {
            return false;
        }
        psize = sizeof(protocol_ctrl_ack);
        break;
    case    proto_connect: {
        protocol_connect* p = (protocol_connect*)header;
        if (sizeof(protocol_connect)+p->data_size*sizeof(ui8) != size) {
            return false;
        }
    }
    psize = sizeof(protocol_connect);
    break;
    case    proto_connect_ack:
        if (sizeof(protocol_connect_ack) != size) {
            return false;
        }
        psize = sizeof(protocol_connect_ack);
        break;
    case    proto_disconnect:
        if (sizeof(protocol_disconnect) != size) {
            return false;
        }
        psize = sizeof(protocol_disconnect);
        break;
    case    proto_heartbeat:
        if (sizeof(protocol_heartbeat) != size) {
            return false;
        }
        psize = sizeof(protocol_heartbeat);
        break;
    case     proto_data: {
        protocol_data* p = (protocol_data*)header;
        if (sizeof(protocol_data)+p->data_size*sizeof(ui8) != size) {
            return false;
        }
    }
    psize = sizeof(protocol_data);
    break;
    case     proto_data_noack: {
        protocol_data_noack* p = (protocol_data_noack*)header;
        if (sizeof(protocol_data_noack)+p->data_size*sizeof(ui8) != size) {
            return false;
        }
    }
    psize = sizeof(protocol_data_noack);
    break;
    case     proto_udp_data: {
        protocol_udp_data* p = (protocol_udp_data*)header;
        if (sizeof(protocol_udp_data)+p->data_size*sizeof(ui8) != size) {
            return false;
        }
    }
    psize = sizeof(protocol_udp_data);
    break;
    }

    if (psize > size) {
        return false;
    }

    ui8 checksum = CHECKSUM_SEED;
    ui8* c = (ui8*)header;
    ++c;
    --psize;
    for (ui16 i = 0; i < psize; ++i) {
        checksum ^= *c++;
    }
    return checksum == header->check_sum;
}
