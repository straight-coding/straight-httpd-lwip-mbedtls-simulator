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
    var size1 = size;
    var size2 = '';
    if (size >= 1000000000) //GB
    {
        if (compact)
        {
            size2 = '' + (size / 1000000000).toFixed(2) + ' GB';
            return size2;
        }
        size2 += parseInt(size / 1000000000,10) + ',';
    }
    if (size >= 1000000) //MB
    {
        if (compact)
        {
            size2 = '' + (size / 1000000).toFixed(2) + ' MB';
            return size2;
        }
        size2 += parseInt((size%1000000000)/1000000,10) + ',';
    }
    if (size >= 1000) //KB
    {
        if (compact)
        {
            size2 = '' + (size / 1000).toFixed(2) + ' KB';
            return size2;
        }
        size2 += parseInt((size%1000000)/1000,10) + ',';
    }

    if (compact)
    {
        size2 = '' + size + ' B';
        return size2;
    }

    size2 += (size%1000);
    return size2 + 'Bytes';
}

function fileTransfer(opt) 
{
    var _this = this;

    _this.settings = opt;

    _this.file = _this.settings.file;
    _this.xhr = null;
    _this.abtTimer = null;
    _this.fileid = _this.settings.idFile;
    _this.toFolder = _this.settings.toFolder;
    
    var li = document.createElement('li');
        li.setAttribute('id', 'file_' + _this.fileid);

    var htmlFileItem  = '<div class="base row center-item between-content file-item">';
        htmlFileItem += '  <div class="base columns stretch-item">';
        htmlFileItem += '    <div class="base row center-item">';
        htmlFileItem += '      <div class="base truncate file-name">' + _this.file.name + '</div>';
        htmlFileItem += '    </div>';
        htmlFileItem += '    <div class="base row center-item between-content file-size">';
        htmlFileItem += '      <div class="base">'+ formatFileSize(parseInt(_this.file.size, 10), 1) + '</div>';
        htmlFileItem += '      <div id="percent_' + _this.fileid + '" class="base">0&#37;</div>';
        htmlFileItem += '    </div>';
        htmlFileItem += '    <div class="base row center-item file-progress">';
        htmlFileItem += '      <div id="progress_' + _this.fileid + '" class="base row">';
        htmlFileItem += '      </div>';
        htmlFileItem += '    </div>';
        htmlFileItem += '  </div>';
        htmlFileItem += '  <div class="base columns right-item file-op">';
        htmlFileItem += '    <div id="stop_' + _this.fileid + '" class="base" style="font-size: 20px; display: none;">&#9632;</div>';
        htmlFileItem += '    <div id="delete_' + _this.fileid + '" class="base" style="display: none;">&#10006;</div>';
        htmlFileItem += '    <a  id="down_' + _this.fileid + '" href="' + _this.toFolder + encodeURI(_this.file.name) +'" class="base" style="font-size: 20px; display: none;" target="_blank">&#8659;</a>';
        htmlFileItem += '  </div>';
        htmlFileItem += '</div>';

    var div = createElementFromHTML(htmlFileItem);
        li.appendChild(div);
    _this.settings.ulContainer.appendChild(li);

    _this.percent = document.getElementById('percent_' + _this.fileid);
    _this.progress = document.getElementById('progress_' + _this.fileid);

    _this.opStop = document.getElementById('stop_' + _this.fileid);
    _this.opDelete = document.getElementById('delete_' + _this.fileid);
    _this.opDownload = document.getElementById('down_' + _this.fileid);

    _this.progress.style.backgroundColor = '#17a2b8';
    
    _this.xhr = new XMLHttpRequest();
    _this.xhr.upload.addEventListener('progress', function(e) 
    {
        if (_this.abtTimer != null)
            clearTimeout(_this.abtTimer);
        _this.abtTimer = setTimeout(function(){ _this.xhr.abort(); }, _this.settings.timeout*1000);

        if (e.lengthComputable) 
        {
            var rate = (e.loaded / e.total) * 100;
            _this.progress.style.width = rate + '%';
            _this.percent.innerHTML = '<span>' + parseInt(rate,10) + '&#37;</span>';
        }
    }, false);

    _this.opStop.addEventListener('click', function(e)
    {
        _this.xhr.abort();
    }, false);

    _this.opDelete.addEventListener('click', function(e)
    {
        var item = e.target.closest('li');
        item.remove();
    }, false);

    _this.xhr.onreadystatechange = function() 
    { 
        console.log('onreadystatechange', _this.xhr.status); 
    }
    _this.xhr.onloadstart = function() 
    { 
        console.log('onloadstart', _this.xhr.status); 
    }
    _this.xhr.onload  = function() 
    { 
        console.log('onload', _this.xhr.status); 
        _this.onSucceeded(); 
    }
    _this.xhr.onabort = function () 
    { 
        console.log('onabort', _this.xhr.status); 
        _this.onFailed(); 
    }
    _this.xhr.onerror = function () 
    { 
        console.log('onerror', _this.xhr.status); 
        _this.onFailed(1); 
    }
    _this.xhr.ontimeout = function() 
    { 
        console.log('ontimeout', _this.xhr.status); 
        _this.onFailed();
    }
    _this.xhr.onloadend = function() 
    { 
        console.log('onloadend', _this.xhr.status); 
    }
}

fileTransfer.prototype.upload = function()
{
    var _this= this;

    try
    {
        _this.opStop.style.display = ''; //show stop

        _this.xhr.open('post', _this.settings.urlUpload, true);

        _this.xhr.setRequestHeader('Content-Type', 'multipart/form-data');
        _this.xhr.setRequestHeader('X-File-Name', Base64Encode(_this.file.name));
        _this.xhr.setRequestHeader('X-File-Size', _this.file.size);
        _this.xhr.setRequestHeader('X-File-Type', _this.file.type);
        //_this.xhr.setRequestHeader('Expect', '100-continue');

        _this.xhr.send(_this.file);
    }
    catch(e)
    {
        console.log(e);
    }

    _this.abtTimer = setTimeout(function(){ _this.xhr.abort(); }, _this.settings.timeout*1000);
}

fileTransfer.prototype.onSucceeded = function()
{
    var _this = this;
    
    if (_this.abtTimer != null)
        clearTimeout(_this.abtTimer);
    _this.abtTimer = null;

    _this.progress.style.width = '100%';
    _this.percent.innerHTML = '<span>100&#37;</span>';

    _this.percent.style.color = '#28a745';
    _this.progress.style.backgroundColor = '#28a745';

    _this.opStop.style.display = 'none'; //hide stop
    _this.opDelete.style.display = ''; //show delete
    _this.opDownload.style.display = ''; //show download

    if (typeof _this.settings.onCompleted == 'function')
        _this.settings.onCompleted(_this);
}

fileTransfer.prototype.onFailed = function(check_session)
{
    var _this = this;

    if (_this.abtTimer != null)
        clearTimeout(_this.abtTimer);
    _this.abtTimer = null;

    _this.percent.style.color = '#dc3545';
    _this.progress.style.backgroundColor = '#dc3545';

    _this.opStop.style.display = 'none'; //hide stop
    _this.opDelete.style.display = ''; //show delete

    if (typeof _this.settings.onCompleted == 'function')
        _this.settings.onCompleted(_this);

    if (!check_session)
        return;

    var session = new XMLHttpRequest();
    session.onload = function()
    {
        console.log('session.onload', session.status); 
        if (session.status != 200)
            location.href='/';
    }
    session.onerror = function () 
    {
        console.log('session.onerror', session.status); 
        location.href='/';
    }
    session.open("GET", "/auth/session_check.cgi");
    session.send();
}
