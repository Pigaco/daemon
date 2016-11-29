$(window).ready(function() {
    var xhr = new XMLHttpRequest(),
            method = "GET",
            url = "/devkit/" + devkit_token + "/log/";

    xhr.open(method, url, true);
    xhr.onreadystatechange = function () {
                if(xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
                            }
            };
    xhr.send();
});
