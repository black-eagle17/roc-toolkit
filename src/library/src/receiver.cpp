/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/receiver.h"

#include "address_helpers.h"
#include "config_helpers.h"

#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_peer/receiver.h"

using namespace roc;

int roc_receiver_open(roc_context* context,
                      const roc_receiver_config* config,
                      roc_receiver** result) {
    roc_log(LogInfo, "roc_receiver_open: opening receiver");

    if (!result) {
        roc_log(LogError, "roc_receiver_open: invalid arguments: result is null");
        return -1;
    }

    if (!context) {
        roc_log(LogError, "roc_receiver_open: invalid arguments: context is null");
        return -1;
    }

    peer::Context* imp_context = (peer::Context*)context;

    if (!config) {
        roc_log(LogError, "roc_receiver_open: invalid arguments: config is null");
        return -1;
    }

    pipeline::ReceiverConfig imp_config;
    if (!api::receiver_config_from_user(imp_config, *config)) {
        roc_log(LogError, "roc_receiver_open: invalid arguments: bad config");
        return -1;
    }

    core::ScopedPtr<peer::Receiver> imp_receiver(
        new (imp_context->allocator()) peer::Receiver(*imp_context, imp_config),
        imp_context->allocator());

    if (!imp_receiver) {
        roc_log(LogError, "roc_receiver_open: can't allocate receiver");
        return -1;
    }

    if (!imp_receiver->valid()) {
        roc_log(LogError, "roc_receiver_open: can't initialize receiver");
        return -1;
    }

    *result = (roc_receiver*)imp_receiver.release();
    return 0;
}

int roc_receiver_bind(roc_receiver* receiver,
                      roc_port_type type,
                      roc_protocol proto,
                      roc_address* address) {
    if (!receiver) {
        roc_log(LogError, "roc_receiver_bind: invalid arguments: receiver is null");
        return -1;
    }

    peer::Receiver* imp_receiver = (peer::Receiver*)receiver;

    if (!address) {
        roc_log(LogError, "roc_receiver_bind: invalid arguments: address is null");
        return -1;
    }

    address::SocketAddr& imp_address = api::get_socket_addr(address);
    if (!imp_address.has_host_port()) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: bad address");
        return -1;
    }

    address::EndpointType imp_endpoint_type;
    if (!api::endpoint_type_from_user(imp_endpoint_type, type)) {
        roc_log(LogError, "roc_receiver_bind: invalid arguments: bad type");
        return -1;
    }

    address::EndpointProtocol imp_endpoint_proto;
    if (!api::endpoint_proto_from_user(imp_endpoint_proto, proto)) {
        roc_log(LogError, "roc_receiver_bind: invalid arguments: bad protocol");
        return -1;
    }

    if (!imp_receiver->bind(imp_endpoint_type, imp_endpoint_proto, imp_address)) {
        roc_log(LogError, "roc_receiver_bind: bind failed");
        return -1;
    }

    return 0;
}

int roc_receiver_read(roc_receiver* receiver, roc_frame* frame) {
    if (!receiver) {
        roc_log(LogError, "roc_receiver_read: invalid arguments: receiver is null");
        return -1;
    }

    peer::Receiver* imp_receiver = (peer::Receiver*)receiver;

    sndio::ISource& imp_source = imp_receiver->source();

    if (!frame) {
        roc_log(LogError, "roc_receiver_read: invalid arguments: frame is null");
        return -1;
    }

    if (frame->samples_size == 0) {
        return 0;
    }

    const size_t factor = imp_source.num_channels() * sizeof(float);

    if (frame->samples_size % factor != 0) {
        roc_log(LogError,
                "roc_receiver_read: invalid arguments: # of samples should be "
                "multiple of # of %u",
                (unsigned)factor);
        return -1;
    }

    if (!frame->samples) {
        roc_log(LogError, "roc_receiver_read: invalid arguments: samples is null");
        return -1;
    }

    audio::Frame imp_frame((float*)frame->samples, frame->samples_size / sizeof(float));

    if (!imp_source.read(imp_frame)) {
        roc_log(LogError, "roc_receiver_read: got unexpected eof from source");
        return -1;
    }

    return 0;
}

int roc_receiver_close(roc_receiver* receiver) {
    if (!receiver) {
        roc_log(LogError, "roc_receiver_close: invalid arguments: receiver is null");
        return -1;
    }

    peer::Receiver* imp_receiver = (peer::Receiver*)receiver;

    imp_receiver->destroy();

    roc_log(LogInfo, "roc_receiver_close: closed receiver");

    return 0;
}