/*
  fileTransfer.js
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 04, 2020

  Dependencies: 
    share.js
    share.css
    fileTransfer.css
    base64js.min.js
*/

'use strict';

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
        li.style.width = "100%";
        li.setAttribute('id', 'file_' + _this.fileid);

    var htmlFileItem  = '<div class="base row center-item between-content file-item">';
        htmlFileItem += '  <div class="base columns stretch-item">';
        htmlFileItem += '    <div class="base row center-item">';
        htmlFileItem += '      <div class="base truncate file-name" title="' + _this.file.name + '">' + _this.file.name + '</div>';
        htmlFileItem += '    </div>';
        htmlFileItem += '    <div class="base row center-item between-content file-size">';
        htmlFileItem += '      <div class="base">';
        htmlFileItem += '        <div class="base">'+ formatFileSize(parseInt(_this.file.size, 10), 1) + '</div>&nbsp;&nbsp;';
        htmlFileItem += '        <div id="speed_' + _this.fileid + '" class="base"></div>';
        htmlFileItem += '      </div>';
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
        htmlFileItem += '    <a id="down_' + _this.fileid + '" href="' + _this.toFolder + encodeURI(_this.file.name) +'" class="base" style="font-size: 20px; display: none;" target="_blank">&#8659;</a>';
        htmlFileItem += '  </div>';
        htmlFileItem += '</div>';

    var div = createElementFromHTML(htmlFileItem);
        li.appendChild(div);
    _this.settings.ulContainer.appendChild(li);

    _this.speed = document.getElementById('speed_' + _this.fileid);
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
            _this.updateProgress(false, e.loaded, e.total);
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

        _this.updateProgress(true);
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

fileTransfer.prototype.updateProgress = function(final, loaded, total)
{
    var _this= this;

    if (!_this.speedMarks)
        _this.speedMarks = [];

    if (total)
    {
        _this.speedMarks.push({
            tick: (new Date()).getTime(),
            loaded: loaded,
            total: total
        });

        var rate = (100 * loaded / total);
        _this.progress.style.width = rate + '%';
        _this.percent.innerHTML = '<span>' + parseInt(rate,10) + '&#37;</span>';
    }

    var qLen = _this.speedMarks.length;
    if (qLen > 0)
    {
        var tickStart = _this.speedMarks[0].tick;
        var sizeStart = _this.speedMarks[0].loaded;
        if ((qLen > 10) && !final)
        {
            tickStart = _this.speedMarks[qLen-10].tick;
            sizeStart = _this.speedMarks[qLen-10].loaded;
        }
        var ticks = (_this.speedMarks[qLen-1].tick - tickStart)/1000;
        var bytes = _this.speedMarks[qLen-1].loaded - sizeStart;
        if (ticks > 0)
        {
            _this.speed.innerHTML = '&nbsp;&nbsp;@&nbsp;<span>' + formatFileSize(bytes/ticks, 1) + '/s</span>';
        }
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
