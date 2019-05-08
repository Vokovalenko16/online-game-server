function stylizer(str) {
  var val = "" + str + "";
  if (val.length < 2) {
    val = "0" + str;
  } else {
    val = str;
  }

  return val;
}

("use strict");

WebApp.controller("GameController", [
  "$scope",
  "$rootScope",
  "HttpService",
  function($scope, $rootScope, HttpService) {
    // Game level list
    $scope.levels = {};
    // Running Game list
    $scope.playings = {};

    // game console
    $scope.console = null;

    $scope.new_game = null;

    $scope.requestPara = null;
    $scope.formatStartDate = null;
    $scope.formatStartTime = null;
    $scope.formatEndDate = null;
    $scope.formatEndTime = null;
    var dt = new Date();

    $scope.curDate =
      dt.getFullYear() +
      "-" +
      stylizer(dt.getMonth() + 1) +
      "-" +
      stylizer(dt.getDate());
    $scope.curTime = stylizer(dt.getHours()) + ":" + stylizer(dt.getMinutes());
    $scope.baseUri = getBaseUri(document.URL);

    // Get All Game Obj
    HttpService.request(
      $scope.baseUri + "level/uploaded_list",
      "POST",
      "{}"
    ).then(function(res) {
      if (res.succeeded == true) $scope.levels = res.level;
      else toastr.error("Failed to get all games", "Fail");
    });

    // Get All Running game
    HttpService.request($scope.baseUri + "game/list", "POST", "{}").then(
      function(res) {
        if (res.succeeded == true) $scope.playings = res.games;
        else toastr.error("Failed to get running games", "Fail");
      }
    );

    // page load event
    $scope.$on("$viewContentLoaded", function() {
      $rootScope.$broadcast("set_active", { page: "games" });
    });

    // View console output
    $scope.view_console = function(gid) {
      //'{"id":' + gid + '}'
      HttpService.request(
        $scope.baseUri + "game/console",
        "POST",
        '{"id":' + gid + "}"
      ).then(function(res) {
        if (res.succeeded == true) {
          $scope.console = res;

          $("#console_modal").modal("show");
        } else {
          toastr.error("Failed to get game records", "Fail");
        }
      });
    };

    // terminate game
    $scope.terminate = function(gid, $index) {
      HttpService.request(
        $scope.baseUri + "game/terminate",
        "POST",
        '{"id":' + gid + "}"
      ).then(function(res) {
        if (res.succeeded == true) {
          $scope.playings.splice($index, 1);
          toastr.success("Succeed to terminate game", "Success");
        } else {
          toastr.error("Failed to terminate game", "Fail");
        }
      });
    };

    // Create game
    $scope.show_create = function() {
      $("#create_modal").modal("show");
    };
    $scope.create_level = function() {
      $scope.formatStartDate = moment($scope.new_game.start_date).format(
        "YYYY-MM-DD"
      );
      $scope.formatStartTime = moment($scope.new_game.start_time).format(
        "HH:mm:ss"
      );

      $scope.formatEndDate = moment($scope.new_game.end_date).format(
        "YYYY-MM-DD"
      );
      $scope.formatEndTime = moment($scope.new_game.end_time).format(
        "HH:mm:ss"
      );

      var x = {
        game_name: $scope.new_game.game_name,
        level: $scope.new_game.level,
        game_mode: Number($scope.new_game.mode),
        start_tm: $scope.formatStartDate + " " + $scope.formatStartTime,
        end_tm: $scope.formatEndDate + " " + $scope.formatEndTime
      };
      $scope.requestPara = JSON.stringify(x);

      /*$scope.requestPara = '{"level":"' + $scope.new_game.level + '",' +
                           '"game_mode":' + $scope.new_game.mode + ',' +
                           '"start_tm":"' + $scope.formatStartDate + ' ' + $scope.formatStartTime + '",' +
                           '"end_tm":"' + $scope.formatEndDate + ' ' + $scope.formatEndTime + '"}'; 
    var requestParaJSON = JSON.parse($scope.requestPara);
    console.log(requestParaJSON);
    exit();*/

      HttpService.request(
        $scope.baseUri + "game/create",
        "POST",
        $scope.requestPara
      ).then(function(res) {
        if (res.succeeded == true) {
          toastr.success(
            "Succeed to create a game.(job_id=" + res.job_id + ")",
            "Success"
          );
          $scope.levels.push({ name: $scope.new_game.level });
          HttpService.request($scope.baseUri + "game/list", "POST", "{}").then(
            function(res) {
              if (res.succeeded == true) $scope.playings = res.games;
              else toastr.error("Failed to create game.", "Fail");
            }
          );
          $("#create_modal").modal("hide");
        } else {
          toastr.error("Failed to create game", "Fail");
        }
      });
    };
  }
]);
