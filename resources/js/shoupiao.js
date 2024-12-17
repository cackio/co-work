const other = document.querySelector(".other");
const title = document.querySelector(".title");
const down = document.querySelector(".down");
const titletext = document.getElementById("title");
window.addEventListener("scroll", () => {
  const height = window.scrollY;
  title.style.fontSize = `${160 + 0.15 * height}px`;
  title.style.marginTop = ` -${1.5 * height}px`;
  title.style.backgroundPosition = `${0.7 * height}px ${0.7 * height}px`;
  const topheight = down.getBoundingClientRect().top;
  if (topheight <= 180) {
    titletext.style.opacity = "1";
    other.classList.add("right");
    title.style.opacity = "0";
  } else {
    titletext.style.opacity = "0";
    other.classList.remove("right");
    title.style.opacity = "1";
  }
});

// 登录
let isLogin = false;
const loginBtn = document.querySelector(".login");
const background = document.querySelector(".background");

function openLogin() {
  const loginContent = document.createElement("div");
  loginContent.className = "login-content";
  loginContent.innerHTML = `
  <h1>Login</h1>
  <input type="text" placeholder="Username" class="userName">
  <input type="password" placeholder="Password" class="passWord">
  <button class="login-confirm">Login</button>
  `;
  background.appendChild(loginContent);
  loginContent.style.display = "block";
  //遮罩
  const loginOver = document.createElement("div");
  loginOver.className = "login-over";
  document.body.appendChild(loginOver);
  loginOver.style.display = "block";

  const userName = document.querySelector(".userName");
  const passWord = document.querySelector(".passWord");
  const confirmBtn = document.querySelector(".login-confirm");
  confirmBtn.addEventListener("click", () => {
    //登录成功/失败判断
    if (userName.value == "uestc" && passWord.value == "123456") {
      closeLogin(loginContent, loginOver);
      success();
    } else if (userName.value === "" || passWord.value === "") {
      empty();
    } else if (userName.value !== "uestc" || passWord.value !== "123456") {
      lose();
      userName.value = "";
      passWord.value = "";
    }
  });
}

//打开登录弹窗
loginBtn.addEventListener("click", () => {
  if (!isLogin) {
    openLogin();
  } else {
    return;
  }
});

//关闭登录弹窗函数
function closeLogin(loginContent, loginOver) {
  loginContent.style.display = "none";
  loginOver.style.display = "none";
}

//登录成功提示函数
function success() {
  const success = document.createElement("div");
  success.className = "success";
  success.innerHTML = `
  <div class="success-box">
    <span class="success-content">登录成功！</span>
  </div>
  `;
  background.appendChild(success);

  isLogin = true;

  setTimeout(() => {
    background.removeChild(success);
  }, 2000);
}

//登录失败提示函数
function lose() {
  const lose = document.createElement("div");
  lose.className = "lose";
  lose.innerHTML = `
  <div class="lose-box">
    <span class="lose-content">用户名或密码错误！</span>
  </div>
  `;
  background.appendChild(lose);

  setTimeout(() => {
    background.removeChild(lose);
  }, 2000);
}

//未输入用户名/密码提示函数
function empty() {
  const empty = document.createElement("div");
  empty.className = "empty";
  empty.innerHTML = `
  <div class="empty-box">
    <span class="empty-content">请输入用户名和密码!</span>
  </div>
  `;
  background.appendChild(empty);

  setTimeout(() => {
    background.removeChild(empty);
  }, 2000);
}

// 提交购票信息函数
function submitTickets() {
  const checkBoxes = document.querySelectorAll('input[name="ticket"]:checked');
  const value = Array.from(checkBoxes).map((checkbox) => checkbox.value);
  checkBoxes.forEach((check) => {
    const ticketIndex = parseInt(check.value, 10) - 1; // 通过票的值确定余票的索引
    if (ticketIndex >= 0 && ticketIndex < left.length) {
      left[ticketIndex] -= 1; // 减少余票数量
    }
    const close = check.closest("tr").querySelector(".left");
    if (close) {
      close.innerText = `余票：${left[ticketIndex]}`;
    }
  });
  //console.log(value);
  if (value.length == 0) {
    alert("请至少选择一种票！");
    return;
  }

  let data = {
    tickets: value,
  };

  fetch('/buy_tickets', {
    method: 'POST',
    headers: {
        'Content-Type': 'application/json',
        'Cache-Control':'no-cache, no-store, must-revalidate'
    },
    body: JSON.stringify(data)  // 发送请求的 body，包含票务信息
}).then(response => {
    if (!response.ok) {
        throw new Error('未响应');
    }
    return response.text();  // 使用 .text() 来解析 HTML 响应
}).then(htmlContent => {
    // 处理服务器返回的 HTML 内容
    // 例如，插入返回的 HTML 到页面的某个元素中
    //document.getElementById('response-container').innerHTML = htmlContent; // 假设你有一个容器来显示返回的 HTML 内容
    
    alert('提交成功!');
}).catch(error => {
    console.error('请求失败:', error);
    alert('提交失败!');
});
}

//如果已登录，按下提交按钮可以提交购票信息，否则跳转至登录界面
const submitBtn = document.querySelector(".submit-btn");
submitBtn.addEventListener("click", () => {
  if (isLogin) {
    submitTickets();
  } else {
    openLogin();
  }
});
//购票成功后余票数量减少
let left = [34, 65, 72, 61, 37, 52, 45, 76, 84, 45, 46, 78, 45, 51, 86, 74, 26];
const alltr = document.querySelectorAll("tr");
let i = 0;
alltr.forEach((tr) => {
  const td = document.createElement("td");
  td.classList.add("left");
  tr.appendChild(td);
  td.innerText = `余票：${left[i]}`;
  i++;
});
