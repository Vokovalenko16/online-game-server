function getBaseUri(uri)
{
    var protocolEnd;
    protocolEnd = uri.indexOf('://');
    if (protocolEnd < 0)
        return null;
    
    var siteEnd;
    siteEnd = uri.indexOf('/', protocolEnd + 3);
    if (siteEnd < 0)
        return uri + '/';
    else
        return uri.substring(0, siteEnd + 1);
}
