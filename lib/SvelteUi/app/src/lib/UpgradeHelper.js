export function upgradeWarningText(board) {
    return 'WARNING: ' + board + ' must be connected to an external power supply during firmware upgrade. Failure to do so may cause power-down during upload resulting in non-functioning unit.'
}

export async function upgrade(expected_version) {
    const response = await fetch('/upgrade?expected_version='+expected_version, {
        method: 'POST'
    });
    await response.json();
}
  
export function getNextVersion(currentVersion, releases_orig) {
    if(/^v\d{1,2}\.\d{1,2}\.\d{1,2}$/.test(currentVersion)) {
        let v = currentVersion.substring(1).split('.');
        let v_major = parseInt(v[0]);
        let v_minor = parseInt(v[1]);
        let v_patch = parseInt(v[2]);

        let releases = [...releases_orig];
        releases.reverse();
        let next_patch;
        let next_minor;
        let next_major;
        for(let i = 0; i < releases.length; i++) {
            let release = releases[i];
            let ver2 = release.tag_name;
            let v2 = ver2.substring(1).split('.');
            let v2_major = parseInt(v2[0]);
            let v2_minor = parseInt(v2[1]);
            let v2_patch = parseInt(v2[2]);

            if(v2_major == v_major) {
                if(v2_minor == v_minor) {
                    if(v2_patch > v_patch) {
                        next_patch = release;
                    }
                } else if(v2_minor == v_minor+1) {
                    next_minor = release;
                }
            } else if(v2_major == v_major+1) {
                if(next_major) {
                    let mv = next_major.tag_name.substring(1).split('.');
                    let mv_major = parseInt(mv[0]);
                    let mv_minor = parseInt(mv[1]);
                    let mv_patch = parseInt(mv[2]);
                    if(v2_minor == mv_minor) {
                        next_major = release;
                    }
                } else {
                    next_major = release;
                }
            }
        };
        if(next_minor) {
            return next_minor;
        } else if(next_major) {
            return next_major;
        } else if(next_patch) {
            return next_patch;
        }
        return false;
    } else {
        return releases_orig[0];
    }
}
