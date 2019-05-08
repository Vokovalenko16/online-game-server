WebApp.controller("UserController", [
  "$scope",
  "$rootScope",
  "HttpService",
  function($scope, $rootScope, HttpService) {
    $scope.user_ids = [];

    $scope.new_user = null;

    $scope.baseUri = getBaseUri(document.URL);

    $scope.set_password = null;
    $scope.password_hash = null;
    HttpService.request($scope.baseUri + "user/list", "POST", "{}").then(
      function(res) {
        $scope.user_ids = res.user_ids;
      }
    );

    $scope.$on("$viewContentLoaded", function() {
      $rootScope.$broadcast("set_active", { page: "users" });
    });

    // Get the user detail information
    $scope.view_detail = function(userid) {
      HttpService.request(
        $scope.baseUri + "user/info",
        "POST",
        '{"id":' + userid + "}"
      ).then(function(res) {
        if (res.succeeded == true) {
          $scope.selected_user = res.user;
          $("#view_modal").modal("show");
        } else {
          toastr.error("Failed to get user info", "Error");
        }
      });
    };

    // Create User
    $scope.show_create = function() {
      $("#create_modal").modal("show");
    };

    $scope.create_user = function() {
      var x =
        '{"login_name":"' +
        $scope.new_user.name +
        '",' +
        '"active":' +
        $scope.new_user.active +
        "}";
      var x = {
        login_name: $scope.new_user.name,
        active: Boolean($scope.new_user.active)
      };
      $scope.requestPara = JSON.stringify(x);
      HttpService.request(
        $scope.baseUri + "user/create",
        "POST",
        $scope.requestPara
      ).then(function(res) {
        if (res.succeeded == true) {
          toastr.success("user_id=" + res.id, "Success");
          $scope.user_ids.push(res.id);
          $("#create_modal").modal("hide");
        } else {
          toastr.error("Failed to create new User", "Error");
        }
      });
    };

    $scope.modify_user = function() {
      $scope.requestPara =
        '{ "id":' +
        $scope.selected_user.id +
        "," +
        '"login_name":"' +
        $scope.selected_user.login_name +
        '",' +
        '"active":' +
        $scope.selected_user.active +
        "}";
      HttpService.request(
        $scope.baseUri + "user/modify",
        "POST",
        $scope.requestPara
      ).then(function(res) {
        if (res.succeeded == true) {
          $scope.user_ids.push({ id: res.id });
          $("#view_modal").modal("hide");
        } else {
          toastr.error("Failed to create new User", "Error");
        }
      });
    };
    // Set Password
    $scope.show_password_modal = function(user_id) {
      $scope.set_password.user_id = user_id;
      $("#setPassword_modal").modal("show");
    };

    $scope.set_password = function() {
      $scope.password_hash = CryptoJS.SHA256($scope.set_password.password)
        .toString()
        .toUpperCase();
      $scope.requestPara =
        '{"id":' +
        $scope.set_password.user_id +
        "," +
        '"password_hash":"' +
        $scope.password_hash +
        '"}';
      HttpService.request(
        $scope.baseUri + "user/set_password_hash",
        "POST",
        $scope.requestPara
      ).then(function(res) {
        if (res.succeeded == true) {
          toastr.success("Success", "Success");
          $("#setPassword_modal").modal("hide");
        } else {
          toastr.error("Failed", "Error");
        }
      });
    };
  }
]);
