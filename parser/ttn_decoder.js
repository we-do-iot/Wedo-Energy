/**
 * EDC Energy Meter Payload Decoder for The Things Network (TTN) V3
 * 
 * This decoder handles TLV (Type-Length-Value) encoded payloads from 
 * the EDC LoRaWAN energy meter.
 * 
 * @param {Object} input - Input object from TTN
 * @param {Uint8Array} input.bytes - Raw payload bytes
 * @param {number} input.fPort - LoRaWAN port number
 * @returns {Object} Decoded payload object
 */
function decodeUplink(input) {
    var bytes = input.bytes;
    var fPort = input.fPort;

    // Range test detection (port 2, 5 bytes, starts with 0xFF)
    if (fPort === 2 && bytes.length === 5 && bytes[0] === 0xFF) {
        var timestamp = readUInt32BE(bytes.slice(1, 5));
        return {
            data: {
                message_type: "range_test",
                timestamp: timestamp
            }
        };
    }

    try {
        return {
            data: edcEnergy(bytes)
        };
    } catch (e) {
        return {
            errors: [String(e)]
        };
    }
}

/**
 * Encode downlink command for TTN V3
 * 
 * @param {Object} input - Input object from TTN
 * @param {Object} input.data - Data to encode
 * @returns {Object} Encoded bytes and fPort
 */
function encodeDownlink(input) {
    var data = input.data;
    var bytes = [];
    var fPort = 85; // CONFIG_PORT

    if (data.command === "set_reporting_interval") {
        // CMD: 0xFF03 + interval (Little-Endian)
        var interval = data.interval_seconds || 0;
        bytes = [0xFF, 0x03, interval & 0xFF, (interval >> 8) & 0xFF];
    } else if (data.command === "reset") {
        // CMD: 0xFF10FF
        bytes = [0xFF, 0x10, 0xFF];
    } else if (data.command === "factory_reset") {
        // CMD: 0xFF99FF
        bytes = [0xFF, 0x99, 0xFF];
    } else {
        return {
            errors: ["Unknown command: " + data.command]
        };
    }

    return {
        bytes: bytes,
        fPort: fPort
    };
}

// ============= Helper Functions =============

function readUInt32BE(bytes) {
    return ((bytes[0] << 24) >>> 0) + ((bytes[1] << 16) >>> 0) + ((bytes[2] << 8) >>> 0) + (bytes[3] >>> 0);
}

function readUInt24BE(bytes) {
    return ((bytes[0] << 16) >>> 0) + ((bytes[1] << 8) >>> 0) + (bytes[2] >>> 0);
}

function readUInt16BE(bytes) {
    return ((bytes[0] << 8) >>> 0) + (bytes[1] >>> 0);
}

// ============= Main Decoder =============

function edcEnergy(bytes) {
    var decoded = {};
    for (var i = 0; i < bytes.length;) {
        var channel_id = bytes[i++];

        if (channel_id === 0x02) { decoded.battery = bytes[i]; i += 1; }
        else if (channel_id === 0x03) { decoded.read_error = (bytes[i] === 0 ? 0 : 1); i += 1; }
        else if (channel_id === 0x04) { decoded.network_state = (bytes[i] === 0 ? 0 : 1); i += 1; }
        else if (channel_id === 0x05) { decoded.firmware = bytes[i]; i += 1; }

        else if (channel_id === 0x0a) { decoded.active_energy = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }
        else if (channel_id === 0x0b) { decoded.reactive_energy = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }
        else if (channel_id === 0x0c) { decoded.apparent_energy = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }

        else if (channel_id === 0x0d) { decoded.reactive_qa = readUInt24BE(bytes.slice(i, i + 3)) / 1000; i += 3; }
        else if (channel_id === 0x0e) { decoded.reactive_qb = readUInt24BE(bytes.slice(i, i + 3)) / 1000; i += 3; }

        else if (channel_id === 0x14) { decoded.i1 = readUInt24BE(bytes.slice(i, i + 3)) / 10; i += 3; }
        else if (channel_id === 0x15) { decoded.i2 = readUInt24BE(bytes.slice(i, i + 3)) / 10; i += 3; }
        else if (channel_id === 0x16) { decoded.i3 = readUInt24BE(bytes.slice(i, i + 3)) / 10; i += 3; }

        else if (channel_id === 0x1e) { decoded.v1 = readUInt16BE(bytes.slice(i, i + 2)) / 10; i += 2; }
        else if (channel_id === 0x1f) { decoded.v2 = readUInt16BE(bytes.slice(i, i + 2)) / 10; i += 2; }
        else if (channel_id === 0x20) { decoded.v3 = readUInt16BE(bytes.slice(i, i + 2)) / 10; i += 2; }

        else if (channel_id === 0x28) { decoded.peak_demand = readUInt16BE(bytes.slice(i, i + 2)); i += 2; }
        else if (channel_id === 0x29) { decoded.last_demand = readUInt24BE(bytes.slice(i, i + 3)); i += 3; }

        else if (channel_id === 0x32) { decoded.fpl1 = bytes[i] / 100; i += 1; }
        else if (channel_id === 0x33) { decoded.fpl2 = bytes[i] / 100; i += 1; }
        else if (channel_id === 0x34) { decoded.fpl3 = bytes[i] / 100; i += 1; }
        else if (channel_id === 0x35) { decoded.fplt = bytes[i] / 100; i += 1; }

        else if (channel_id === 0x3c) { decoded.active_consumed = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }
        else if (channel_id === 0x3d) { decoded.active_generated = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }
        else if (channel_id === 0x3e) { decoded.reactive_consumed = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }
        else if (channel_id === 0x3f) { decoded.reactive_generated = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }

        else if (channel_id === 0x46) { decoded.active_energy_last_period = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }
        else if (channel_id === 0x47) { decoded.reactive_energy_last_period = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }
        else if (channel_id === 0x48) { decoded.reactive_consumption_last_period = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }
        else if (channel_id === 0x49) { decoded.reactive_generated_last_period = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }
        else if (channel_id === 0x4a) { decoded.peak_demand_last_period = readUInt16BE(bytes.slice(i, i + 2)); i += 2; }
        else if (channel_id === 0x4b) { decoded.active_energy_last_period_alt = readUInt16BE(bytes.slice(i, i + 2)); i += 2; }

        else if (channel_id === 0x50) { decoded.statf1 = bytes[i]; i += 1; }
        else if (channel_id === 0x51) { decoded.statf2 = bytes[i]; i += 1; }

        else if (channel_id === 0x5a) { decoded.serial_number = readUInt32BE(bytes.slice(i, i + 4)); i += 4; }
        else if (channel_id === 0x5b) {
            var sb = bytes.slice(i, i + 8);
            decoded.serial_number_str = String.fromCharCode.apply(null, sb);
            i += 8;
        }

        else { /* unknown channel: break to avoid infinite loop */ break; }
    }
    return decoded;
}
