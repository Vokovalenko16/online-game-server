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
WebApp.controller("SettleController", [
  "$scope",
  "$rootScope",
  "HttpService",
  function($scope, $rootScope, http) {
    $scope.settle = [];
    $scope.baseUri = getBaseUri(document.URL);

    var dt = new Date();

    $scope.startDate =
      dt.getFullYear() + "-" + stylizer(dt.getMonth() + 1) + "-" + stylizer(1);
    $scope.endDate =
      dt.getFullYear() +
      "-" +
      stylizer(dt.getMonth() + 1) +
      "-" +
      stylizer(dt.getDate());
    $scope.settle.startDate = $scope.startDate;
    $scope.settle.endDate = $scope.endDate;
    http.request($scope.baseUri + "#/settle", "POST", "{}").then(function(res) {
      //$scope.jobs = res.jobs;
    });

    $scope.$on("$viewContentLoaded", function() {
      $rootScope.$broadcast("set_active", { page: "settle" });
    });

    // Get the job detail information
    $scope.get_detail = function(usr_id) {
      var data = {
        user: usr_id,
        start_date: $scope.settle.startDate,
        end_date: $scope.settle.endDate
      };
      http
        .request(
          $scope.baseUri + "settle/by_user",
          "POST",
          JSON.stringify(data)
        )
        .then(function(res) {
          if (res.succeeded == true) {
            $scope.detail_rows = res.rows;
            //$('#view_modal').modal('show');
          } else {
            toastr.error("Failed.", "Fail");
          }
        });
    };

    // Cancel the job
    $scope.get_total = function() {
      var data = {
        start_date: $scope.settle.startDate,
        end_date: $scope.settle.endDate
      };
      http
        .request($scope.baseUri + "settle/total", "POST", JSON.stringify(data))
        .then(function(res) {
          if (res.succeeded == true) {
            $scope.rows = res.rows;
            $scope.total_amount = res.total;
          } else {
            toastr.error("Failed.", "Fail");
          }
        });
    };
  }
]);
