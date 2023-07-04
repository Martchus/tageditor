export function query(method, url) {
    let request = new XMLHttpRequest();
    let result = null;
    request.onreadystatechange = function() {
        if (request.readyState === XMLHttpRequest.DONE) {
            utility.exit(0);
            result = {
                status: request.status,
                headers: request.getAllResponseHeaders(),
                contentType: request.responseType,
                content: request.response
            };
        }
    };
    request.open(method, url);
    request.send();
    utility.exec();
    return result;
}

export function get(url) {
    return query("GET", url);
}
