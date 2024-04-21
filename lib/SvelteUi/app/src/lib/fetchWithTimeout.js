
let queueMap = {}
let queue = [];

async function fetchWithTimeoutReal(resource, options = {}) {
    const { timeout = 8000 } = options;
    
    const controller = new AbortController();
    const id = setTimeout(() => controller.abort(), timeout);
    const response = await fetch(resource, {
      ...options,
      signal: controller.signal  
    });
    clearTimeout(id);
    return response;
}

let eatTimeout;
async function eatQueue() {
    if(queue.length) {
        let item = queue.shift();
        delete queueMap[item.resource];
        try {
            let response = await fetchWithTimeoutReal(item.resource, item.options);
            for(let i in item.callbacks) {
                item.callbacks[i](response.clone());
            }
        } catch(e) {
            console.error("Error calling " + item.resource, e);
            for(let i in item.callbacks) {
                item.callbacks[i]();
            }
        }
    }
    if(eatTimeout) clearTimeout(eatTimeout);
    eatTimeout = setTimeout(eatQueue, 100);
};
eatQueue();

export default async function fetchWithTimeout(resource, options = {}) {
    let item;
    if(queueMap[resource]) {
        item = queueMap[resource];
    } else {
        item = {
            resource: resource,
            options: options,
            callbacks: []
        };
        queueMap[resource] = item;
        queue.push(item);
    }
    let promise = new Promise((cb) => item.callbacks.push(cb));
    return promise;
}