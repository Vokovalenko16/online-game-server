"use strict";

WebApp.controller("UploadController", [
  "$scope",
  "$rootScope",
  "HttpService",
  function($scope, $rootScope, HttpService) {
    $scope.baseUri = getBaseUri(document.URL);

    $scope.releases = null;
    $scope.requestPara = null;

    HttpService.request(
      $scope.baseUri + "child_admin",
      "POST",
      JSON.stringify({
        server_name: "download",
        request_path: "/release/list",
        request_data: {}
      })
    ).then(function(res) {
      $scope.releases = res.releases;
    });

    $scope.$on("$viewContentLoaded", function() {
      $rootScope.$broadcast("set_active", { page: "upload" });
    });

    // Active release
    $scope.set_active = function(rid, $index, flag) {
      HttpService.request(
        $scope.baseUri + "child_admin",
        "POST",
        JSON.stringify({
          server_name: "download",
          request_path: "/release/set_active",
          request_data: {
            id: Number(rid),
            active: Number(flag)
          }
        })
      ).then(function(res) {
        if (res.succeeded == true) {
          //$scope.releases.splice($index, 1);
          toastr.success("Activated.", "Success");
        } else {
          toastr.error("Failed to activate", "Fail");
        }
      });
    };

    // Add new release
    $scope.add_release = function() {
      HttpService.request(
        $scope.baseUri + "child_admin",
        "POST",
        JSON.stringify({
          server_name: "download",
          request_path: "/release/add",
          request_data: {
            id: Number($scope.release.id),
            name: $scope.release.name,
            logical_path: "",
            physical_path: $scope.release.physical_path
          }
        })
      ).then(function(res) {
        if (res.succeed == true) {
          toastr.success("", "Success");
        } else {
          toastr.error("", "Fail");
        }
      });
    };
  }
]);
