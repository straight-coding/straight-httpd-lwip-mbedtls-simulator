/*
  share.js
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 07, 2020
*/

'use strict';

function Base64Encode(str, encoding = 'utf-8') 
{
    var bytes = new(TextEncoder || TextEncoderLite)(encoding).encode(str);
    return base64js.fromByteArray(bytes);
}

function Base64Decode(str, encoding = 'utf-8') 
{
    var bytes = base64js.toByteArray(str);
    return new(TextDecoder || TextDecoderLite)(encoding).decode(bytes);
}

function createElementFromHTML(htmlString) 
{
    var div = document.createElement('div');
        div.innerHTML = htmlString.trim();
    return div.firstChild; 
}

function formatFileSize(size, compact)
{
    var size1 = parseInt(size, 10);
    var size2 = '';
    if (size1 >= 1000000000) //GB
    {
        if (compact)
        {
            size2 = '' + (size1 / 1000000000).toFixed(2) + ' GB';
            return size2;
        }
        size2 += parseInt(size1 / 1000000000,10) + ',';
    }
    if (size1 >= 1000000) //MB
    {
        if (compact)
        {
            size2 = '' + (size1 / 1000000).toFixed(2) + ' MB';
            return size2;
        }
        size2 += parseInt((size1%1000000000)/1000000,10) + ',';
    }
    if (size1 >= 1000) //KB
    {
        if (compact)
        {
            size2 = '' + (size1 / 1000).toFixed(2) + ' KB';
            return size2;
        }
        size2 += parseInt((size1%1000000)/1000,10) + ',';
    }

    if (compact)
    {
        size2 = '' + size1 + ' B';
        return size2;
    }

    size2 += (size1 % 1000);
    return size2 + 'Bytes';
}

function xhrWrapper(opt) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function (e) {
        if (typeof opt.success == 'function')
        opt.success(xhr);
    }
    xhr.onprogress = function (e) {
        if (typeof opt.progress == 'function')
        opt.progress(xhr);
    }
    xhr.ontimeout = function(e) {
        if (typeof opt.timeout == 'function')
        opt.timeout(xhr);
        else if (typeof opt.error == 'function')
        opt.error(xhr);
    }
    xhr.onerror = function (e) {
        if (typeof opt.error == 'function')
        opt.error(xhr);
    }
    xhr.onloadend = function(e) {
        if (typeof opt.complete == 'function')
            opt.complete(xhr);
    }

    if (opt.timeout)
        xhr.timeout = opt.timeout;
    if (opt.contentType)
        xhr.setRequestHeader('Content-type', opt.contentType);

    xhr.open(opt.method, opt.url);
    if (opt.data)
        xhr.send(opt.data);
    else
        xhr.send();
}
