function login() {
    var username = document.getElementById('usernameInput').value;
    var password = document.getElementById('passwordInput').value;

    var xhr = new XMLHttpRequest();
    xhr.open('POST', 'login');
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.onreadystatechange = function () {
        if (xhr.readyState === 4 && xhr.status === 200) {
            var response = xhr.responseText;
            document.open();
            document.write(response);
            document.close();
        } else {
            var errorMessage = document.getElementById("error-message");
            errorMessage.style.display = "block";
            errorMessage.textContent = "用户名不存在或密码错误";
            return false;
        }
        return true;
    };

    var data = "username=" + username + "&password=" + password;
    xhr.send(data);
}