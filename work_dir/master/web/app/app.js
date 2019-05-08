var WebApp = angular.module("WebApp", [
  "ui.router",
  "ui.bootstrap",
  "ngSanitize"
]);

WebApp.constant('Globals', {
  master_host: 'http://10.0.10.61:3000/',
  upload_host: 'http://10.0.10.61:3000/'
});

/* Setup App Main Controller */
WebApp.controller('AppController', ['$scope', '$rootScope', function ($scope, $rootScope) {
  $scope.$on('$viewContentLoaded', function () {

  });
}]);

/* Setup Layout Part - Header */
WebApp.controller('HeaderController', ['$scope', function ($scope) {
  $scope.$on('$includeContentLoaded', function () {

  });
}]);

/* Setup Layout Part - Sidebar */
WebApp.controller('SidebarController', ['$scope', '$rootScope', function ($scope, $rootScope) {
  $scope.page = 'users'
  $scope.$on('set_active', function(e, param) {
    $scope.page = param.page;
  });
}]);

/* Setup Layout Part - Sidebar */
WebApp.controller('PageHeadController', ['$scope', function ($scope) {
  $scope.page = 'users'
  $scope.$on('set_active', function(e, param) {
    $scope.page = param.page;
  });
}]);

/* Setup Layout Part - Footer */
WebApp.controller('FooterController', ['$scope', function ($scope) {
  $scope.$on('$includeContentLoaded', function () {

  });
}]);

WebApp.config(['$stateProvider', '$urlRouterProvider', function ($stateProvider, $urlRouterProvider) {

  // Redirect any unmatched url
  $urlRouterProvider.otherwise("/jobs");

  $stateProvider

    // Dashboard
    .state('users', {
      url: "/users",
      templateUrl: "users.html",
      controller: "UserController"
    })
    .state('jobs', {
      url: "/jobs",
      templateUrl: "jobs.html",
      controller: "JobController"
    })
    .state('games', {
      url: "/games",
      templateUrl: "games.html",
      controller: "GameController"
    })
    .state('levels', {
      url: "/levels",
      templateUrl: "levels.html",
      controller: "LevelController"
    })
    .state('upload', {
      url: "/upload",
      templateUrl: "upload.html",
      controller: "UploadController"
    })
    .state('settle', {
      url: "/settle",
      templateUrl: "settle.html",
      controller: "SettleController"
    });
  }]);

WebApp.run(["$rootScope", "$state", function ($rootScope, $state) {
  $rootScope.$state = $state; // state to be accessed from view
}]);