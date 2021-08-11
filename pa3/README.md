- [5 分] 提交格式正确，包括所有需要的文件。代码可以正常编译、执行。

- [10 分] 参数插值: 正确插值颜色、法向量、纹理坐标、位置 (Shading Position) 并将它们传递给 fragment_shader_payload.

  **实现了与pa2类似的插值算法，实现法向量、颜色纹理、颜色插值**

```c++
    auto v = t.toVector4();

    int top = ceil(std::max(v[0].y(), std::max(v[1].y(), v[2].y())));
    int bottom = floor(std::min(v[0].y(), std::min(v[1].y(), v[2].y())));
    int left = floor(std::min(v[0].x(), std::min(v[1].x(), v[2].x())));
    int right = ceil(std::max(v[0].x(), std::max(v[1].x(), v[2].x())));
    
    for (int x = left; x <= right; x++)
    {
        for (int y = bottom; y <= top; y++)
        {
            if(insideTriangle(x, y, t.v))
            {
                //Depth interpolated
                auto[alpha,beta,gamma] = computeBarycentric2D(x,y,t.v);
                float Z = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                float zp = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                zp *= Z;
                
                if(zp < depth_buf[get_index(x,y)])
                {
                    // color
                    auto interpolated_color = interpolate(alpha,beta,gamma,t.color[0],t.color[1],t.color[2],1);
                    // normal
                    auto interpolated_normal = interpolate(alpha,beta,gamma,t.normal[0],t.normal[1],t.normal[2],1).normalized();
                    // texcoords
                    auto interpolated_texcoords = interpolate(alpha,beta,gamma,t.tex_coords[0],t.tex_coords[1],t.tex_coords[2],1);
                    // shadingcoords
                    auto interpolated_shadingcoords = interpolate(alpha,beta,gamma,view_pos[0],view_pos[1],view_pos[2],1);

                    // interpolated result
                    fragment_shader_payload payload(interpolated_color, interpolated_normal, interpolated_texcoords, texture ? &*texture : nullptr);
                    payload.view_pos = interpolated_shadingcoords;
                    auto pixel_color = fragment_shader(payload);
                    // set depth
                    depth_buf[get_index(x,y)] = zp;
                    // set color
                    set_pixel(Eigen::Vector2i(x,y),pixel_color);
                }
            }
        }
    }
```

- [20 分]Blinn-phong 反射模型: 正确实现 phong_fragment_shader 对应的 反射模型。

  ![](C:\Users\wangxin16\Desktop\pa3\images\2.png)

  ```c++
      for (auto& light : lights)
      {
          // light's dir
          Eigen::Vector3f light_dir = light.position - point;
          // view's dir
          Eigen::Vector3f view_dir = eye_pos - point;
          // r
          auto r = light_dir.dot(light_dir);
  
          // ambient
          Eigen::Vector3f la = ka.cwiseProduct(amb_light_intensity);
          // diffuse
          Eigen::Vector3f ld = kd.cwiseProduct(light.intensity / r);
          ld *= std::max(0.0f, normal.normalized().dot(light_dir.normalized()));
          // specular
          Eigen::Vector3f h = (light_dir + view_dir).normalized();
          Eigen::Vector3f ls = ks.cwiseProduct(light.intensity / r);
          ls *= std::pow(std::max(0.0f, normal.normalized().dot(h)), p);
          
          result_color += (la+ld+ls);
      }
  ```

- [5 分] Texture mapping: 将 phong_fragment_shader 的代码拷贝到 texture_fragment_shader, 在此基础上正确实现 Texture Mapping.

  **在Blinn-Phong的基础上实现纹理**

  ![](C:\Users\wangxin16\Desktop\pa3\images\3.png)

```c++
    Eigen::Vector3f return_color = {0, 0, 0};
    if (payload.texture)
    {
        // TODO: Get the texture value at the texture coordinates of the current fragment
        return_color = payload.texture->getColor(payload.tex_coords.x(),payload.tex_coords.y());
    }
    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = texture_color / 255.f;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = texture_color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
```

- [10 分] Bump mapping 与 Displacement mapping: 正确实现 Bump mapping 与 Displacement mapping.

**在Blinn-Phong基础上实现Bump mapping**

![](C:\Users\wangxin16\Desktop\pa3\images\4.png)

```c++
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;


    float kh = 0.2, kn = 0.1;

    float x = normal.x();
    float y = normal.y();
    float z = normal.z();
    Eigen::Vector3f t{x*y/std::sqrt(x*x+z*z), std::sqrt(x*x+z*z), z*y/std::sqrt(x*x+z*z)};
    Eigen::Vector3f b = normal.cross(t);
    Eigen::Matrix3f TBN;
    TBN <<  t.x(),b.x(),normal.x(),
            t.y(),b.y(),normal.y(),
            t.z(),b.z(),normal.z();
    
    float u = payload.tex_coords.x();
    float v = payload.tex_coords.y();
    float w = payload.texture->width;
    float h = payload.texture->height;

    float dU = kh*kn*(payload.texture->getColor(u+1.f/w,v).norm()-payload.texture->getColor(u,v).norm());
    float dV = kh*kn*(payload.texture->getColor(u,v+1.f/h).norm()-payload.texture->getColor(u,v).norm());

    Eigen::Vector3f n{-dU,-dV,1.0f};

    normal = TBN * n;
    
    Eigen::Vector3f result_color = {0, 0, 0};
    result_color = normal.normalized();
```

**在Bump mapping的基础上实现 Displacement mapping**

![](C:\Users\wangxin16\Desktop\pa3\images\5.png)

```c++
   Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    float kh = 0.2, kn = 0.1;
    
    float x = normal.x();
    float y = normal.y();
    float z = normal.z();
    Eigen::Vector3f t{x*y/std::sqrt(x*x+z*z), std::sqrt(x*x+z*z), z*y/std::sqrt(x*x+z*z)};
    Eigen::Vector3f b = normal.cross(t);
    Eigen::Matrix3f TBN;
    TBN <<  t.x(),b.x(),normal.x(),
            t.y(),b.y(),normal.y(),
            t.z(),b.z(),normal.z();
    
    float u = payload.tex_coords.x();
    float v = payload.tex_coords.y();
    float w = payload.texture->width;
    float h = payload.texture->height;

    float dU = kh*kn*(payload.texture->getColor(u+1.f/w,v).norm()-payload.texture->getColor(u,v).norm());
    float dV = kh*kn*(payload.texture->getColor(u,v+1.f/h).norm()-payload.texture->getColor(u,v).norm());

    Eigen::Vector3f n{-dU,-dV,1.0f};

    point += (kn*normal*payload.texture->getColor(u,v).norm());

    normal = TBN * n;
    normal = normal.normalized();
    
    Eigen::Vector3f result_color = {0, 0, 0};
```

