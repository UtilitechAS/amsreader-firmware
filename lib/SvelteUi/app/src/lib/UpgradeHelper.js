export async function upgrade(version) {
    const data = new URLSearchParams()
    data.append('version', version.tag_name);
    const response = await fetch('/upgrade', {
        method: 'POST',
        body: data
    });
    let res = (await response.json())
}
  
export function getNextVersion(currentVersion, releases) {
    if(/^v\d{1,2}\.\d{1,2}\.\d{1,2}$/.test(currentVersion)) {
        var v = currentVersion.substring(1).split('.');
        var v_major = parseInt(v[0]);
        var v_minor = parseInt(v[1]);
        var v_patch = parseInt(v[2]);

        releases.reverse();
        var next_patch;
        var next_minor;
        var next_major;
        for(var i = 0; i < releases.length; i++) {
            var release = releases[i];
            var ver2 = release.tag_name;
            var v2 = ver2.substring(1).split('.');
            var v2_major = parseInt(v2[0]);
            var v2_minor = parseInt(v2[1]);
            var v2_patch = parseInt(v2[2]);

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
                    var mv = next_major.tag_name.substring(1).split('.');
                    var mv_major = parseInt(mv[0]);
                    var mv_minor = parseInt(mv[1]);
                    var mv_patch = parseInt(mv[2]);
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
        return releases[0];
    }
}
