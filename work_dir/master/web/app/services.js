WebApp.service('HttpService', ['$q', function ($q) {
  return {
    request: function (endpoint, method, data) {
      var defered = $q.defer();
      $.ajax({
        url: endpoint,
        type: method,
        data: data,
        dataType: 'JSON',
        success: function (res) {
          defered.resolve(res);
        },
        error: function (err) {
          defered.reject();
        }
      });

      return defered.promise;
    }
  };
}]);
