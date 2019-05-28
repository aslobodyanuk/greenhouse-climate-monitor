function fillLatestData(latestData)
    {
      $("#latestTempValueCell").text(latestData.Temperature);
      $("#latestHumidityValueCell").text(latestData.Humidity);
      $("#latestLightnessValueCell").text(latestData.Light);
      $("#currentTimeLatestReading").text(latestData.Time);
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

    $(document).ready(function () {
      console.log("Loading charts...");
      getChartData();
      getLatestData();
    });