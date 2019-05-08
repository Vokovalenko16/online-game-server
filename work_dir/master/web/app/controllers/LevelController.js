"use strict";

WebApp.controller("LevelController", [
  "$scope",
  "$rootScope",
  "HttpService",
  function($scope, $rootScope, HttpService) {
    // Uploaded level list
    $scope.levels = null;
    $scope.level = null;
    $scope.baseUri = getBaseUri(document.URL);
    HttpService.request($scope.baseUri + "game/list_levels", "POST", "{}").then(
      function(res) {
        if (res.succeeded == true) $scope.uploaded_levels = res.levels;
        else toastr.error("Failed to display uploaded game", "실패");
      }
    );
    HttpService.request(
      $scope.baseUri + "level/uploaded_list",
      "POST",
      "{}"
    ).then(function(res) {
      if (res.succeeded == true) $scope.levels = res.level;
      else toastr.error("Failed to display registered map", "Fail");
    });

    // page load event
    $scope.$on("$viewContentLoaded", function() {
      $rootScope.$broadcast("set_active", { page: "levels" });
    });

    $scope.upload_level = function() {
      var x = { level: $scope.level.name };
      $scope.requestPara = JSON.stringify(x);

      HttpService.request(
        $scope.baseUri + "level/upload",
        "POST",
        $scope.requestPara
      ).then(function(res) {
        if (res.succeeded == true) {
          toastr.success("Succeed to upload", "Success");
          $scope.levels.push({ name: $scope.level.name });
        } else {
          toastr.error("Failed to upload", "Fail");
        }
      });
    };
  }
]);
