export const PARITY_CODES = {
    '7N1': 2,
    '8N1': 3,
    '8N2': 7,
    '7E1': 10,
    '8E1': 11
};

function bufferForBaud(baud) {
    if (!baud || Number.isNaN(baud)) {
        return undefined;
    }
    if (baud >= 115200) {
        return 512;
    }
    if (baud >= 57600) {
        return 384;
    }
    if (baud >= 19200) {
        return 256;
    }
    return 192;
}

export const meterPresets = [
    {
        id: 'aidon-rj12',
        manufacturer: 'Aidon',
        label: 'Aidon – P1 RJ12 (6442/6490/6492)',
        interface: 'RJ12 P1',
        baud: 115200,
        parity: '8N1',
        invert: true,
        encryption: 'none',
        notes: 'Built-in P1 interface used by Norwegian DSOs.',
        parser: 0
    },
    {
        id: 'aidon-rj45-han',
        manufacturer: 'Aidon',
        label: 'Aidon – HAN RJ45 (65xx/6474/6476/6477/6479/6484)',
        interface: 'RJ45 HAN-NVE',
        baud: 2400,
        parity: '8E1',
        invert: false,
        encryption: 'optional',
        notes: 'HAN-NVE adapter for Norwegian HAN ports.',
        parser: 0
    },
    {
        id: 'kaifa-rj45',
        manufacturer: 'Kaifa/Nuri',
        label: 'Kaifa/Nuri – HAN RJ45 (MA105/MA304)',
        interface: 'RJ45 HAN-NVE',
        baud: 2400,
        parity: '8E1',
        invert: false,
        encryption: 'optional',
        notes: 'Standard HAN profile in Norway.',
        parser: 0
    },
    {
        id: 'kaifa-rj12',
        manufacturer: 'Kaifa/Nuri',
        label: 'Kaifa/Nuri – P1 RJ12 (MA309MH4)',
        interface: 'RJ12',
        baud: 2400,
        parity: '8N1',
        invert: true,
        encryption: 'optional',
        notes: 'P1 breakout offered by select Norwegian operators.',
        parser: 0
    },
    {
        id: 'kamstrup-omnipower-han',
        manufacturer: 'Kamstrup',
        label: 'Kamstrup Omnipower – HAN adapter',
        interface: 'RJ45 HAN-NVE',
        baud: 2400,
        parity: '8N1',
        invert: false,
        encryption: 'optional',
        notes: 'Pow-K / HAN adapter for Norwegian Omnipower installs.',
        parser: 0
    },
    {
        id: 'kamstrup-omnipower-p1',
        manufacturer: 'Kamstrup',
        label: 'Kamstrup Omnipower – P1 adapter',
        interface: 'RJ12 P1',
        baud: 115200,
        parity: '8N1',
        invert: true,
        encryption: 'none',
        notes: 'P1 conversion cable provided by some Norwegian grid companies.',
        parser: 0
    }
];

export function getMeterPresetById(id) {
    return meterPresets.find((preset) => preset.id === id);
}

export function buildMeterStateFromPreset(currentState, preset) {
    if (!preset) {
        return currentState;
    }

    const next = {
        ...currentState
    };

    if (typeof preset.parser === 'number') {
        next.parser = preset.parser;
    }
    if (typeof preset.baud === 'number') {
        next.baud = preset.baud;
    }
    if (preset.parity) {
        next.parity = PARITY_CODES[preset.parity] ?? next.parity;
    }
    if (typeof preset.invert === 'boolean') {
        next.invert = preset.invert;
    }
    const suggestedBuffer = bufferForBaud(preset.baud);
    if (suggestedBuffer) {
        next.buffer = suggestedBuffer;
    }
    if (preset.encryption === 'none') {
        next.encrypted = false;
        next.encryptionKey = '';
        next.authenticationKey = '';
    } else if (preset.encryption === 'required') {
        next.encrypted = true;
    }

    next.appliedPresetId = preset.id;
    return next;
}

export function createMeterStateFromConfiguration(config) {
    if (!config) {
        return {
            source: 1,
            parser: 0,
            baud: 2400,
            parity: PARITY_CODES['8N1'],
            invert: false,
            distributionSystem: 2,
            mainFuse: 63,
            production: 0,
            buffer: 256,
            encrypted: false,
            encryptionKey: '',
            authenticationKey: '',
            multipliers: {
                enabled: false,
                watt: 1,
                volt: 1,
                amp: 1,
                kwh: 1
            }
        };
    }

    return {
        source: config?.o ?? 1,
        parser: config?.a ?? 0,
        baud: config?.b ?? 2400,
        parity: config?.p ?? PARITY_CODES['8N1'],
        invert: !!config?.i,
        distributionSystem: config?.d ?? 2,
        mainFuse: config?.f ?? 63,
        production: config?.r ?? 0,
        buffer: config?.s ?? 256,
        encrypted: !!config?.e?.e,
        encryptionKey: config?.e?.k ?? '',
        authenticationKey: config?.e?.a ?? '',
        multipliers: {
            enabled: !!config?.m?.e,
            watt: config?.m?.w ?? 1,
            volt: config?.m?.v ?? 1,
            amp: config?.m?.a ?? 1,
            kwh: config?.m?.c ?? 1
        }
    };
}

export function applyMeterStateToConfiguration(configuration, state) {
    if (!configuration || !state) {
        return configuration;
    }
    const next = { ...configuration };
    next.a = state.parser;
    next.b = state.baud;
    next.p = state.parity;
    next.i = state.invert;
    next.d = state.distributionSystem;
    next.f = state.mainFuse;
    next.r = state.production;
    next.s = state.buffer;
    next.o = state.source ?? 1;

    next.e = {
        ...next.e,
        e: !!state.encrypted,
        k: state.encryptionKey ?? '',
        a: state.authenticationKey ?? ''
    };

    next.m = {
        ...next.m,
        e: !!state.multipliers?.enabled,
        w: state.multipliers?.watt ?? 1,
        v: state.multipliers?.volt ?? 1,
        a: state.multipliers?.amp ?? 1,
        c: state.multipliers?.kwh ?? 1
    };

    return next;
}

export function describePresetSummary(preset) {
    if (!preset) {
        return '';
    }
    const parts = [];
    if (preset.baud) {
        parts.push(`${preset.baud} baud`);
    }
    if (preset.parity) {
        parts.push(preset.parity);
    }
    if (typeof preset.invert === 'boolean') {
        parts.push(preset.invert ? 'Inverted' : 'Normal');
    }
    if (preset.interface) {
        parts.push(preset.interface);
    }
    return parts.join(' • ');
}
