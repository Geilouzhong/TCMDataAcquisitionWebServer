var todayRecordHeader = ['序号', '操作', '就诊ID', '患者姓名', '患者ID', '手机号', '接诊时间', '诊断时间'];
var RecordListHeader = ['序号', '操作', '就诊ID', '患者姓名', '患者ID', '接诊时间', '诊断时间'];
var fieldMap = {
    "面诊": "faceDiagnose",
    "舌诊": "tongueDiagnose",
    "脉诊": "pulseDiagnose",
    "医生主诉": "doctorChiefComplaint",
    "诊断结果": "medicalResult",
    "治疗方案": "treatment",
    "备注": "remark",
};
const PageSize = 10;

function loadDoctorName() {
    sendHttpRequest("doctors/getDoctorName", loadName, []);
}

function loadName(responseText) {
    var data = responseText;
    try {
        data = JSON.parse(responseText);
    } catch (error) {

    }
    var nameBox = document.getElementById("welcome");
    nameBox.innerText = data["医生姓名"];
}

loadDoctorName();

function changePassword() {
    var newPassword = prompt('输入新密码');

    if (newPassword) {
        body = generateHttpPostBody("doctorPassword=" + newPassword);
        sendHttpRequest("doctors/updateDoctorPassword", null, [], "POST", "application/x-www-form-urlencoded", body)
            .then(function (result) {
                logout();
            })
            .catch(function (error) {
                alert("修改失败")
            });
    }
}

function logout() {
    sendHttpRequest("doctors/logout", reloadLogin, [], "GET", 'text/html', "");
}

function reloadLogin(responseText) {
    document.open();
    document.write(responseText);
    document.close();
}

function getParameter(str, parameter) {
    var reg = new RegExp("(\\?|&)" + parameter + "=([^&]*)");
    var r = str.match(reg);
    if (r != null && r[2]) {
        return r[2];
    }
    return null;
}

function getTodayList(url) {
    var pageNum = document.getElementById("hiddenPageNum");
    pageNum.innerHTML = getParameter(url, "pageIndex");
    var thead = createTableHeader(todayRecordHeader);
    sendHttpRequest(url, generateTable, todayRecordHeader)
        .then(function (result) {
            let currentUrl = window.location.href;
            history.replaceState(null, null, currentUrl.split("#")[0] + "#/todayrecords");
        })
        .catch(function (error) {

        });

    sendHttpRequest("records/getTodayQueryNum", generatePageBox, []);
}

function getRecordsList(url) {
    var pageNum = document.getElementById("hiddenPageNum");
    pageNum.innerHTML = getParameter(url, "pageIndex");
    var thead = createTableHeader(RecordListHeader);
    sendHttpRequest(url, generateTable, RecordListHeader)
        .then(function (result) {
            let currentUrl = window.location.href;
            history.replaceState(null, null, currentUrl.split("#")[0] + "#/records");
        })
        .catch(function (error) {

        });

    sendHttpRequest("records/getQueryNum", generatePageBox, []);
}

function getRecord(url) {
    sendHttpRequest(url, displayRecord, [])
        .then(function (result) {
            let currentUrl = window.location.href;
            var IDSpan = document.getElementById("receptionId");
            history.replaceState(null, null, currentUrl + "/" + IDSpan.innerText.split(": ")[1]);
        })
        .catch(function (error) {
            alert("获取失败");
        });
}

function updateRecord(url) {
    var parentDiv = document.getElementById('userDetail');
    var inputElements = parentDiv.getElementsByTagName('input');
    var bodys = [];
    for (var i = 0; i < inputElements.length; i++) {
        if (inputElements[i].id !== "") {
            bodys.push(inputElements[i].id + "=" + inputElements[i].value);
        }
    }
    body = generateHttpPostBody.apply(null, bodys);
    sendHttpRequest(url, null, [], "POST", "application/x-www-form-urlencoded", body)
        .then(function (result) {
            alert("保存成功")
        })
        .catch(function (error) {
            alert("保存失败")
        });
}

function displayRecord(responseText) {
    var data = responseText;
    try {
        data = JSON.parse(responseText);
    } catch (error) {

    }
    var container = document.getElementById("data-container");
    container.innerHTML = "";

    var content = document.createElement("div");
    content.id = "userDetail";
    container.appendChild(content);

    var userinfobox = document.createElement("div");
    content.appendChild(userinfobox);
    userinfobox.classList.add("personal-info");

    var nameSpan = document.createElement("span");
    nameSpan.innerText = data["患者姓名"] + "   ID: " + data["患者ID"];
    var otherSpan = document.createElement("span");
    otherSpan.innerText = data["手机号"];
    userinfobox.appendChild(nameSpan);
    userinfobox.appendChild(otherSpan);

    delete data["患者姓名"];
    delete data["患者ID"];
    delete data["手机号"];

    var diagnosticInfoBox = document.createElement("div");
    content.appendChild(diagnosticInfoBox);
    diagnosticInfoBox.classList.add("personal-info");

    var IDSpan = document.createElement("span");
    IDSpan.innerText = "就诊ID: " + data["就诊ID"];
    IDSpan.id = "receptionId";
    var timeSpan = document.createElement("span");
    timeSpan.innerText = "接诊时间: " + data["接诊时间"];
    var doctorSpan = document.createElement("span");
    doctorSpan.innerText = "诊断医生： " + data["医生姓名"]
    diagnosticInfoBox.appendChild(IDSpan);
    diagnosticInfoBox.appendChild(timeSpan);
    diagnosticInfoBox.appendChild(doctorSpan);

    delete data["就诊ID"];
    delete data["接诊时间"];
    delete data["医生姓名"];

    var infoBox = document.createElement("div");
    diagnosticInfoBox.appendChild(infoBox);

    for (var key in data) {
        var infoDiv = document.createElement("div");
        infoBox.appendChild(infoDiv);
        infoDiv.classList.add("inquiry_info");

        var infoNameDiv = document.createElement("div");
        var infoInput = document.createElement("input");
        infoInput.type = "text";
        if (key == "患者自述") {
            infoInput.readOnly = true;
        }
        else {
            infoInput.id = fieldMap[key];
        }

        infoDiv.appendChild(infoNameDiv);
        infoDiv.appendChild(infoInput);

        infoNameDiv.innerText = key;
        infoInput.value = data[key];
    }

    let currentUrl = window.location.href;
    if (currentUrl.split("#")[1] == "/todayrecords") {
        var saveButton = document.createElement("button");
        diagnosticInfoBox.appendChild(saveButton);
        saveButton.innerHTML = "保存";
        saveButton.onclick = function () {
            const url = window.location.href;
            const segments = url.split("/");
            updateRecord("records/updateRecord?receptionId=" + segments.pop());
        };
    }
}

function clear() {
    var container = document.getElementById("data-container");
    container.innerHTML = "";
    var divBox = document.createElement("div");
    container.appendChild(divBox);
}

function generateHttpPostBody(...params) {
    // 将参数拼接成请求体字符串
    const body = params.join('&');
    return body;
}

function sendHttpRequest(url, action, header, method = "GET", contentType = "application/json", body = "") {
    return new Promise(function (resolve, reject) {
        const xhr = new XMLHttpRequest();
        xhr.open(method, `/${url}`);
        xhr.setRequestHeader("Content-Type", contentType);

        xhr.onload = function () {
            if (xhr.status === 200) {

                if (action != null) {
                    if (header == []) {
                        action(xhr.responseText);
                    }
                    else {
                        action(xhr.responseText, header);
                    }
                }
                resolve(true);

            }
            else if (xhr.status === 401) {
                logout();
            }
            else {
                console.error('请求失败', xhr.statusText);
                if (action != null) {

                }
                reject(false);
            }
        };

        xhr.onerror = function () {
            console.error('请求错误');
            reject(false);
        };

        if (method == "POST") {
            xhr.send(body);
        } else {
            xhr.send();
        }
    });
}

function createTableHeader(headerArray) {
    var container = document.getElementById("data-container");
    container.innerHTML = "";

    var table = document.createElement('table');
    container.appendChild(table);
    table.id = 'table';

    var thead = document.createElement('thead');
    var tr = document.createElement('tr');

    headerArray.forEach(function (headerText) {
        var th = document.createElement('th');
        th.textContent = headerText;
        tr.appendChild(th);
    });

    thead.appendChild(tr);
    table.appendChild(thead);

    return table;
}

function generateTable(responseText, header) {
    var data = responseText;
    try {
        data = JSON.parse(responseText);
    } catch (error) {

    }
    var table = document.getElementById('table');

    // 创建表格内容
    var tbody = document.createElement('tbody');
    var index = PageSize * (parseInt(document.getElementById('hiddenPageNum').innerHTML) - 1);
    data.forEach(function (obj) {
        var tr = document.createElement('tr');
        index++;

        header.forEach(function (key) {
            var td = document.createElement('td');
            if (obj.hasOwnProperty(key)) {
                td.textContent = obj[key];
            }
            else if (key == "操作") {
                var button = document.createElement("button");
                button.innerHTML = "查看";
                button.onclick = function () {
                    getRecord('records/getRecord?receptionId=' + obj["就诊ID"]);
                };
                td.appendChild(button);
            }
            else if (key == "序号") {
                td.textContent = index.toString();
            }
            tr.appendChild(td);
        });

        tbody.appendChild(tr);
    });
    table.appendChild(tbody);
}

function generatePageBox(responseText) {
    data = JSON.parse(responseText);
    var container = document.getElementById("data-container");
    var pageBox = document.createElement("div");
    container.appendChild(pageBox);

    var previousPage = document.createElement("button");
    var nextPage = document.createElement("button");
    var totalPage = document.createElement("span");
    var inputPage = document.createElement("input");
    var dumpButton = document.createElement("button");

    container.appendChild(previousPage);
    container.appendChild(totalPage);
    container.appendChild(nextPage);
    container.appendChild(inputPage);
    container.appendChild(dumpButton);

    previousPage.innerHTML = "上一页";
    nextPage.innerHTML = "下一页";
    dumpButton.innerHTML = "跳转";
    var totalNum = Math.ceil(parseInt(data["total"]) / 10);

    totalPage.innerText = document.getElementById("hiddenPageNum").innerHTML + "/" + totalNum.toString();

    totalPage.id = "totalPage";
    inputPage.id = "inputPage";

    previousPage.onclick = function () {
        var pageInfo = document.getElementById("totalPage").innerText.split("/");
        var curPageNum = pageInfo[0];
        var totalPageNum = pageInfo[1];
        if (parseInt(curPageNum) <= 1) {
            alert("超出页码范围");
            return false;
        }
        var skipPageNum = parseInt(curPageNum) - 1;
        skipPage(skipPageNum.toString());
    }

    nextPage.onclick = function () {
        var pageInfo = document.getElementById("totalPage").innerText.split("/");
        var curPageNum = pageInfo[0];
        var totalPageNum = pageInfo[1];
        console.log(totalPageNum);
        if (parseInt(curPageNum) >= parseInt(totalPageNum)) {
            alert("超出页码范围");
            return false;
        }
        var skipPageNum = parseInt(curPageNum) + 1;
        console.log(skipPageNum);
        skipPage(skipPageNum.toString());
    }

    dumpButton.onclick = function () {
        var pageInfo = document.getElementById("totalPage").innerText.split("/");
        var totalPageNum = pageInfo[1];
        var toSkipNum = document.getElementById("inputPage").value;
        if (parseInt(toSkipNum) > parseInt(totalPageNum) || parseInt(toSkipNum) < 1) {
            alert("超出页码范围");
            return false;
        }
        skipPage(toSkipNum);
    }

}

function skipPage(skipPageNum) {
    let currentUrl = window.location.href;
    if (currentUrl.split("#")[1] == "/todayrecords") {
        url = "records/getTodayUserList?pageIndex=" + skipPageNum + "&pageSize=10";
        getTodayList(url);
    }
    else if (currentUrl.split("#")[1] == "/records") {
        url = "records/getRecordList?pageIndex=" + skipPageNum + "&pageSize=10";
        getRecordsList(url);
    }
}

// 打开对话框
function addPatient() {
    var name = prompt('输入患者ID');

    if (name) {
        body = generateHttpPostBody("patientId=" + name);
        sendHttpRequest("records/addRecord", null, [], "POST", "application/x-www-form-urlencoded", body)
            .catch(function (error) {
                alert("用户不存在")
            });
        getTodayList('records/getTodayUserList?pageIndex=1&pageSize=10');
    }
}