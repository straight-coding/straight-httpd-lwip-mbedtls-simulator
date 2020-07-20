/*
  share.js
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 07, 2020
*/

'use strict';

if (!Element.prototype.matches) {
    Element.prototype.matches = Element.prototype.msMatchesSelector || 
    Element.prototype.webkitMatchesSelector;
}

if (!Element.prototype.closest) {
    Element.prototype.closest = function(s) {
        var el = this;

        do {
            if (Element.prototype.matches.call(el, s)) 
                return el;
            el = el.parentElement || el.parentNode;
        } while (el !== null && el.nodeType === 1);
        return null;
    };
}

function Base64Encode(str, encoding) 
{
    var bytes = new(TextEncoder || TextEncoderLite)(encoding || 'utf-8').encode(str);
    return base64js.fromByteArray(bytes);
}

function Base64Decode(str, encoding) 
{
    var bytes = base64js.toByteArray(str);
    return new(TextDecoder || TextDecoderLite)(encoding || 'utf-8').decode(bytes);
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

    //if (opt.timeout)
        //xhr.timeout = opt.timeout;
    if (opt.contentType)
        xhr.setRequestHeader('Content-type', opt.contentType);

    xhr.open(opt.method, opt.url);
    if (opt.data)
        xhr.send(opt.data);
    else
        xhr.send();
}

function checkSession(opt)
{
    if (!opt)
        return;

    var session = new XMLHttpRequest();
    session.onload = function()
    {
        if (session.status == 200)
        {
            if (typeof opt.alive == 'function')
                opt.alive();
            return;
        }
        
        if (typeof opt.notfound == 'function')
            opt.notfound();
        else
            location.href='/';        
    }
    session.onerror = function () 
    {
        if (typeof opt.notfound == 'function')
            opt.notfound();
        else
            location.href='/';        
    }
    session.open("GET", "/auth/session_check.cgi");
    session.send();
}

function setSessionCHecker()
{
    if (window == top)
    {
        setInterval(function() { 
            checkSession({ alive: function() {} }); 
        }, 30*1000);
    }
}
