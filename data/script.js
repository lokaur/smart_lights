var state = {
  state: 0,
  light: 10,
  current: 0,
};

$("input.btn-check").click(function () {
  var mode = $(this).attr("id");
  sendMode(mode);
});

function sendMode(mode) {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = handleResponse;
  xhr.open("POST", "/mode", true);
  xhr.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
  xhr.send(JSON.stringify({ mode: mode }));
}

function handleResponse() {
  if (this.readyState == 4 && this.status == 200 && this.responseText != "") {
    var result = parseInt(this.responseText, 10);
    state.state = result;
    console.log("state", state);
    updateState();
  }
}

function updateLight() {
  $("p.light-value").text(state.light);
}

function updateState() {
  resetNavColors();
  switch (state.current) {
    case 0:
      $("nav.navbar").addClass("bg-danger");
      break;
    case 1:
      $("nav.navbar").addClass("bg-success");
      break;
  }
}

function updateButtons() {
  resetButtonColors();
  switch (state.state) {
    case 0:
      $(".btn-check#off").attr("checked", "checked");
      break;
    case 1:
      $(".btn-check#on").attr("checked", "checked");
      break;
    case 2:
      $(".btn-check#auto").attr("checked", "checked");
      break;
  }
}

function resetButtonColors() {
  $(".btn-check#off").removeAttr("checked");
  $(".btn-check#on").removeAttr("checked");
  $(".btn-check#auto").removeAttr("checked");
}

function resetNavColors() {
  $("nav.navbar").removeClass("bg-success");
  $("nav.navbar").removeClass("bg-danger");
  $("nav.navbar").removeClass("bg-secondary");
}

function enableControls() {
  $("nav.navbar").removeClass("bg-secondary");
  $(".btn-check#off").removeAttr("disabled");
  $(".btn-check#on").removeAttr("disabled");
  $(".btn-check#auto").removeAttr("disabled");
  $("label[for=off]").removeClass("btn-outline-secondary");
  $("label[for=off]").addClass("btn-outline-danger");
  $("label[for=on]").removeClass("btn-outline-secondary");
  $("label[for=on]").addClass("btn-outline-success");
  $("label[for=auto]").removeClass("btn-outline-secondary");
  $("label[for=auto]").addClass("btn-outline-info");
}

function disableControls() {
  resetColors();
  $("nav.navbar").addClass("bg-secondary");
  $(".btn-check#off").attr("disabled");
  $(".btn-check#on").attr("disabled");
  $(".btn-check#auto").attr("disabled");
  $("label[for=off]").addClass("btn-outline-secondary");
  $("label[for=on]").addClass("btn-outline-secondary");
  $("label[for=auto]").addClass("btn-outline-secondary");
}

if (!!window.EventSource) {
  var source = new EventSource("/events");

  source.addEventListener(
    "open",
    function (e) {
      console.log("Events Connected");
    },
    false
  );

  source.addEventListener(
    "error",
    function (e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
        disableControls();
      }
    },
    false
  );

  source.addEventListener(
    "message",
    function (e) {
      console.log("message", e.data);
    },
    false
  );

  source.addEventListener(
    "light",
    function (e) {
      var light = parseInt(e.data, 10);
      console.log("light", light);
      state.light = light;
      updateLight();
    },
    false
  );

  source.addEventListener(
    "init",
    function (e) {
      state = JSON.parse(e.data);
      console.log("init", state);
      enableControls();
      updateState();
      updateButtons();
      updateLight();
    },
    false
  );

  source.addEventListener(
    "current",
    function (e) {
      var current = parseInt(e.data, 10);
      console.log("current", current);
      state.current = current;
      updateState();
    },
    false
  );
}
