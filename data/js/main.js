var temperatureChart;
var humidityChart;
var lightChart;

function fillLatestData(latestData) {
  $("#latestTempValueCell").text(latestData.Temperature);
  $("#latestHumidityValueCell").text(latestData.Humidity);
  $("#latestLightnessValueCell").text(latestData.Light);
  $("#currentTimeLatestReading").text(latestData.Time);
  $("#uptimeValueCell").text(latestData.UptimeSeconds);
  $("#totalSunTimeCell").text(latestData.TotalSunTime);
  $("#currentDayLengthCell").text(latestData.CurrentDayLength);
}

function getChartData() {
  $.ajax({
    type: "GET",
    url: "/getChartData",
    success: function (data) {
      console.log(data);
      fillCharts(data);
    }
  });
}

function getLatestData() {
  $.ajax({
    type: "GET",
    url: "/getLatestData",
    success: function (data) {
      console.log(data);
      fillLatestData(data);
    }
  });
}

function showConfigurationWindow() {
  $.ajax({
    type: "GET",
    url: "/getConfig",
    success: function (data) {
      console.log(data);
      $("#lattitudeConfigValue").val(data.Latitude);
      $("#longitudeConfigValue").val(data.Longitude);
      $("#desiredTemperatureConfigValue").val(data.DesiredTemperature);
      $("#desiredLightningConfigValue").val(data.DesiredLightning);
      $("#cloudsPercentConfigValue").val(data.CloudsSimulationPercent);
      $("#simulateDataConfigValue").prop("checked", data.SimulateData);

      $("#lattitudeConfigValueLabel").addClass("active");
      $("#longitudeConfigValueLabel").addClass("active");
      $("#desiredTemperatureConfigValueLabel").addClass("active");
      $("#desiredLightningConfigValueLabel").addClass("active");
      $("#cloudsPercentConfigValueLabel").addClass("active");

      $("#configurationModalWindow").modal("show");
    },
    error: function (data) {
      console.log(data);
      fadeInItem($("#configSavedError"));
      setTimeout(() => { fadeOutItem($("#configSavedError")) }, 5000);
    }
  });
}

function saveConfiguration() {
  var isFormValid = true;
  if ($("#lattitudeConfigValue").val() === "") {
    $("#lattitudeConfigValue").addClass("invalid");
    isFormValid = false;
  }
  if ($("#longitudeConfigValue").val() === "") {
    $("#longitudeConfigValue").addClass("invalid");
    isFormValid = false;
  }
  if ($("#desiredTemperatureConfigValue").val() === "") {
    $("#desiredTemperatureConfigValue").addClass("invalid");
    isFormValid = false;
  }
  if ($("#desiredLightningConfigValue").val() === "") {
    $("#desiredLightningConfigValue").addClass("invalid");
    isFormValid = false;
  }
  if ($("#cloudsPercentConfigValue").val() === "") {
    $("#cloudsPercentConfigValue").addClass("invalid");
    isFormValid = false;
  }
  if (isFormValid == false)
    return;

  var reqData = {
    Latitude: $("#lattitudeConfigValue").val(),
    Longitude: $("#longitudeConfigValue").val(),
    DesiredTemperature: $("#desiredTemperatureConfigValue").val(),
    DesiredLightning: $("#desiredLightningConfigValue").val(),
    CloudsSimulationPercent: $("#cloudsPercentConfigValue").val(),
    SimulateData: $("#simulateDataConfigValue").is(":checked")
  };

  $("#configurationModalWindow").modal("hide");

  $.ajax({
    type: "POST",
    url: "/saveConfig",
    data: reqData,
    success: function (data) {
      console.log(data);
      fadeInItem($("#configSavedAlert"));
      setTimeout(() => { fadeOutItem($("#configSavedAlert")) }, 5000);
    },
    error: function (data) {
      console.log(data);
      fadeInItem($("#configSavedError"));
      setTimeout(() => { fadeOutItem($("#configSavedError")) }, 5000);
    }
  });
}

function fadeOutItem(element) {
  element.addClass('animatedHidden').removeClass('animatedVisible');
}

function fadeInItem(element) {
  element.addClass('animatedVisible').removeClass('animatedHidden');
}

function updateDataFromESP() {
  $.ajax({
    type: "GET",
    url: "/getChartData",
    success: function (data) {
      console.log(data);
      updateChartData(temperatureChart, data.TemperatureDayArray);
      updateChartData(humidityChart, data.HumidityDayArray);
      updateChartData(lightChart, data.LightDayArray);
    }
  });

  getLatestData();
}

function updateChartData(chart, data) {
  chart.data.datasets.forEach((dataset) => {
    dataset.data = data;
  });
  chart.update();
}

$(document).ready(function () {
  console.log("Loading charts...");
  getChartData();
  getLatestData();
  setInterval(updateDataFromESP, 10000);
});