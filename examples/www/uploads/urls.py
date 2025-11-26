from django.urls import path
from . import views

app_name = "kanban"

urlpatterns = [
    path("", views.userworkspace, name="userworkspace"),
    path("user_login", views.user_login, name="user_login"),
    path("user_logout", views.user_logout, name="user_logout"),
    path("register", views.register, name="register")
]