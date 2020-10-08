/*
  fileList.js
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 07, 2020

  Dependencies: 
    share.js
    share.css
    fileList.css
*/

'use strict';

function fileList(opt) 
{
    var _this = this;
    _this.settings = opt;

    _this.fileItems = [];
    _this.request();
}

function fileItem(opt)
{
    var _this = this;

    _this.container = opt.container;
    _this.fInfo = opt.fInfo;
    _this.onclick = opt.onclick;

	if (!_this.container.classList.contains('file-list'))
		_this.container.classList.add('file-list');

    var li = document.createElement('li');
        li.style.width = "100%";
        li.setAttribute('id', 'file_' + _this.fInfo.fileid);

    var htmlFileItem  = '<div class="base row center-item between-content file-item">';
        htmlFileItem += '  <div class="base columns stretch-item">';
        htmlFileItem += '    <div class="base row center-item">';
        htmlFileItem += '      <div class="base truncate file-name" title="' + _this.fInfo.name + '">' + _this.fInfo.name + '</div>';
        htmlFileItem += '    </div>';
        htmlFileItem += '    <div class="base row center-item between-content file-size">';
        htmlFileItem += '      <div class="base">';
        if (!_this.fInfo.folder)
            htmlFileItem += '        <div class="base">'+ formatFileSize(parseInt(_this.fInfo.size, 10), 1) + '</div>&nbsp;&nbsp;';
        htmlFileItem += '        <div id="speed_' + _this.fInfo.fileid + '" class="base"></div>';
        htmlFileItem += '      </div>';
        htmlFileItem += '      <div id="percent_' + _this.fInfo.fileid + '" class="base"></div>';
        htmlFileItem += '    </div>';
        htmlFileItem += '  </div>';
        htmlFileItem += '  <div class="base columns right-item file-op">';
        if (_this.fInfo.folder)
        {
            htmlFileItem += '    <div id="dir_' + _this.fInfo.fileid + '" href="' + encodeURI(_this.fInfo.folderPath + _this.fInfo.name) +'" class="base" style="font-size: 20px;">';
            htmlFileItem += '      <img src="plugin/fileList/folder.png" height="25" width="25"/>';
            htmlFileItem += '    </div>';
        }
        else
        {
            htmlFileItem += '    <a id="down_' + _this.fInfo.fileid + '" href="/app/cache' + encodeURI(_this.fInfo.folderPath + _this.fInfo.name) +'" class="base" style="font-size: 20px;" target="_blank">';
            htmlFileItem += '      <img src="plugin/fileList/file.png" height="25" width="25"/>';
            htmlFileItem += '    </a>';
        }
        htmlFileItem += '  </div>';
        htmlFileItem += '</div>';

    var div = createElementFromHTML(htmlFileItem);
        li.appendChild(div);
    _this.container.appendChild(li);

    _this.opDir = document.getElementById('dir_' + _this.fInfo.fileid);
    _this.opDownload = document.getElementById('down_' + _this.fInfo.fileid);
    
    if (_this.opDir)
    {
        _this.opDir.addEventListener('click', function(e)
        {
            var item = e.target.closest('li');

            if (typeof _this.onclick == 'function')
                _this.onclick('dir', item);
        }, false);
    }
}

fileList.prototype.fileClicked = function(tag, item)
{
    var _this = this;

    var lastDir = _this.settings.folderInput.value;
    var nextDir = _this.fileIndex[item.id].name;

    if (lastDir[lastDir.length-1] == '/')
        lastDir = lastDir.substr(0, lastDir.length-1);

    if (nextDir == '..')
    {
        if (!lastDir || (lastDir == '/'))
            nextDir = '/';
        else
        {
            var parent = lastDir.lastIndexOf('/');
            if (parent >= 0)
                nextDir = lastDir.substr(0, parent+1);
            else
                nextDir = '/';
        }
    }
    else
    {
        nextDir = lastDir + '/' + nextDir + '/';
    }

    _this.settings.folderInput.value = nextDir;

    setTimeout(function(){
        _this.request();
    }, 20);
}

fileList.prototype.request = function()
{
    var _this = this;

    _this.fileIndex = {};
    _this.fileItems = [];

    while (_this.settings.container.hasChildNodes()) {  
        _this.settings.container.removeChild(_this.settings.container.firstChild);
    }    

    xhrWrapper({
        method: 'GET',
        url: _this.settings.urlGet + '?path=' + encodeURI(_this.settings.folderInput.value),
        timeout: 60000,
        success: function(e) {
            var obj = JSON.parse(e.response);
            console.log(obj);

            if (obj.data)
            {
                var tree = {};
                for(var i = 0; i < obj.data.length; i ++)
                {
                    obj.data[i].fileid = i+1;
                    obj.data[i].folderPath = _this.settings.folderInput.value;

                    _this.fileIndex['file_' + obj.data[i].fileid] = obj.data[i];

                    if (obj.data[i].folder)
                    {
                        if (!tree.folders)
                            tree.folders = {};
                        tree.folders[obj.data[i].name] = obj.data[i];
                    }
                    else
                    {
                        if (!tree.files)
                            tree.files = {};
                        tree.files[obj.data[i].name] = obj.data[i];
                    }
                }
                for(var name in tree.folders)
                {
                    _this.fileItems.push(new fileItem({
                        container: _this.settings.container,
                        fInfo: tree.folders[name],
                        onclick: function(tag, item) {
                           _this.fileClicked(tag, item);
                        }
                    }));
                }
                for(var name in tree.files)
                {
                    _this.fileItems.push(new fileItem({
                        container: _this.settings.container,
                        fInfo: tree.files[name],
                        onclick: function(tag, item) {
                           _this.fileClicked(tag, item);
                        }
                    }));
                }
            }
        },
        error: function(e) {
            console.log(e.response);
        },
        complete: function(e) {
            console.log(e.readyState, e.status);
        }
    });
}
