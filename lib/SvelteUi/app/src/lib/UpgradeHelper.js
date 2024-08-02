export function upgradeWarningText(board) {
    return 'WARNING: ' + board + ' must be connected to an external power supply during firmware upgrade. Failure to do so may cause power-down during upload resulting in non-functioning unit.'
}

export async function upgrade(expected_version) {
    const response = await fetch('upgrade?expected_version='+expected_version, {
        method: 'POST'
    });
    return await response.json();
}
