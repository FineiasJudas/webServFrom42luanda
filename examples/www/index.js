document.querySelectorAll('.menu li').forEach(item => {
    item.addEventListener('click', event => {
        document.querySelectorAll('.menu li a').forEach(link => {
            link.classList.remove('active');
        });
        item.querySelector('a').classList.add('active');
        if (item.id == 'inicio') {
            window.location.href = './index.html';
        } else if (item.id == 'serviços') {
            window.location.href = './services.html';
        } else if (item.id == 'uploads') {
            window.location.href = './uploads.html';
        } else if (item.id == 'sobre') {
            window.location.href = './about.html';
        } else if (item.id == 'contato') {
            window.location.href = './contact.html';
        }
        else if (item.id == 'sessions') {
            window.location.href = './sessions.html';
        }
    });
});