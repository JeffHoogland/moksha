#include "e.h"
#include "e_mod_main.h"

EINTERN int 
e_drm_output_subpixel_convert(int value)
{
   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   switch (value) 
     {
      case DRM_MODE_SUBPIXEL_NONE:
        return WL_OUTPUT_SUBPIXEL_NONE;
      case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB:
        return WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;
      case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR:
        return WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR;
      case DRM_MODE_SUBPIXEL_VERTICAL_RGB:
        return WL_OUTPUT_SUBPIXEL_VERTICAL_RGB;
      case DRM_MODE_SUBPIXEL_VERTICAL_BGR:
        return WL_OUTPUT_SUBPIXEL_VERTICAL_BGR;
      case DRM_MODE_SUBPIXEL_UNKNOWN:
      default:
          return WL_OUTPUT_SUBPIXEL_UNKNOWN;
     }
}

EINTERN Eina_Bool 
e_drm_output_add_mode(E_Drm_Output *output, drmModeModeInfo *info)
{
   E_Drm_Output_Mode *mode;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(mode = malloc(sizeof(E_Drm_Output_Mode)))) 
     return EINA_FALSE;

   mode->base.flags = 0;
   mode->base.w = info->hdisplay;
   mode->base.h = info->vdisplay;
   mode->base.refresh = info->vrefresh;
   mode->info = *info;

   wl_list_insert(output->base.modes.prev, &mode->base.link);

   return EINA_TRUE;
}

EINTERN void 
e_drm_output_set_modes(E_Drm_Compositor *dcomp)
{
   E_Drm_Output *output;
   E_Drm_Output_Mode *mode;
   int ret = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   wl_list_for_each(output, &dcomp->base.outputs, base.link)
     {
        mode = (E_Drm_Output_Mode *)output->base.current;
        ret = drmModeSetCrtc(dcomp->drm.fd, output->crtc_id, 
                             output->fb_id[output->current ^ 1], 0, 0, 
                             &output->conn_id, 1, &mode->info);
        if (ret < 0)
          printf("Failed to set drm mode: %m\n");
     }
}

EINTERN void 
e_drm_output_scanout_buffer_destroy(struct wl_listener *listener, void *data __UNUSED__)
{
   E_Drm_Output *output;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   output = container_of(listener, E_Drm_Output, scanout_buffer_destroy_listener);
   output->scanout_buffer = NULL;
   if (!output->pending_scanout_buffer)
     e_compositor_schedule_repaint(output->base.compositor);
}

EINTERN void 
e_drm_output_pending_scanout_buffer_destroy(struct wl_listener *listener, void *data __UNUSED__)
{
   E_Drm_Output *output;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   output = container_of(listener, E_Drm_Output, 
                         pending_scanout_buffer_destroy_listener);
   output->pending_scanout_buffer = NULL;
   e_compositor_schedule_repaint(output->base.compositor);
}

EINTERN void 
e_drm_output_repaint(E_Output *base, pixman_region32_t *damage)
{
   E_Drm_Output *output;
   E_Drm_Compositor *dcomp;
   E_Surface *es;
   E_Sprite *sprite;
   unsigned int fb_id = 0;
   int ret = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   output = (E_Drm_Output *)base;
   dcomp = (E_Drm_Compositor *)output->base.compositor;

   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                             GL_RENDERBUFFER, output->rbo[output->current]);
   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
     return;

   e_drm_output_prepare_scanout_surface(output);

   wl_list_for_each_reverse(es, &dcomp->base.surfaces, link)
     e_surface_draw(es, &output->base, damage);

   glFlush();

   output->current ^= 1;

   if (output->pending_fs_surf_fb_id != 0)
     fb_id = output->pending_fs_surf_fb_id;
   else
     fb_id = output->fb_id[output->current ^ 1];

   if (drmModePageFlip(dcomp->drm.fd, output->crtc_id, fb_id, 
                       DRM_MODE_PAGE_FLIP_EVENT, output) < 0)
     return;

   wl_list_for_each(sprite, &dcomp->sprites, link)
     {
        unsigned int flags = 0;
        drmVBlank vbl;

        vbl.request.type = (DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
        vbl.request.sequence = 1;
        if (!e_sprite_crtc_supported(base, sprite->possible_crtcs))
          continue;
        ret = drmModeSetPlane(dcomp->drm.fd, sprite->plane_id, output->crtc_id,
                              sprite->pending_fb_id, flags, sprite->dx, 
                              sprite->dy, sprite->dw, sprite->dh, sprite->sx, 
                              sprite->sy, sprite->sw, sprite->sh);
        if (ret)
          printf("Setplane Failed: %s\n", strerror(errno));

        vbl.request.signal = (unsigned long)sprite;
        ret = drmWaitVBlank(dcomp->drm.fd, &vbl);
        if (ret)
          printf("VBlank event request failed: %s\n", strerror(errno));
     }
}

EINTERN void 
e_drm_output_destroy(E_Output *base)
{
   E_Drm_Output *output;
   E_Drm_Compositor *dcomp;
   drmModeCrtcPtr ocrtc;
   int i = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   output = (E_Drm_Output *)base;
   dcomp = (E_Drm_Compositor *)output->base.compositor;
   ocrtc = output->orig_crtc;

   /* TODO: backlight */
   /* if (base->backlight) */

   e_drm_output_set_cursor(&output->base, NULL);

   drmModeSetCrtc(dcomp->drm.fd, ocrtc->crtc_id, ocrtc->buffer_id, 
                  ocrtc->x, ocrtc->y, &output->conn_id, 1, &ocrtc->mode);
   drmModeFreeCrtc(ocrtc);

   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                             GL_RENDERBUFFER, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
   glDeleteRenderbuffers(2, output->rbo);

   for (i = 0; i < 2; i++)
     {
        drmModeRmFB(dcomp->drm.fd, output->fb_id[i]);
        dcomp->base.destroy_image(dcomp->base.egl_display, output->image[i]);
        gbm_bo_destroy(output->bo[i]);
     }

   dcomp->crtc_alloc &= ~(1 << output->crtc_id);
   dcomp->conn_alloc &= ~(1 << output->conn_id);

   e_output_destroy(&output->base);
   wl_list_remove(&output->base.link);

   free(output);
}

EINTERN void 
e_drm_output_assign_planes(E_Output *base)
{
   E_Compositor *comp;
   E_Surface *es;
   pixman_region32_t overlap, soverlap;
   E_Input_Device *dev;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = base->compositor;
   pixman_region32_init(&overlap);
   wl_list_for_each(es, &comp->surfaces, link)
     {
        pixman_region32_init(&soverlap);
        pixman_region32_intersect(&soverlap, &overlap, &es->transform.box);
        dev = (E_Input_Device *)comp->input_device;
        if (es == dev->sprite)
          {
             e_drm_output_set_cursor_region(base, dev, &soverlap);
             if (!dev->hw_cursor)
               pixman_region32_union(&overlap, &overlap, &es->transform.box);
          }
        else if (!e_drm_output_prepare_overlay_surface(base, es, &soverlap))
          {
             pixman_region32_fini(&es->damage);
             pixman_region32_init(&es->damage);
          }
        else
          pixman_region32_union(&overlap, &overlap, &es->transform.box);

        pixman_region32_fini(&soverlap);
     }
   pixman_region32_fini(&overlap);

   e_drm_output_disable_sprites(base);
}

EINTERN void 
e_drm_output_set_dpms(E_Output *base, E_Dpms_Level level)
{
   E_Drm_Output *output;
   E_Compositor *comp;
   E_Drm_Compositor *dcomp;
   drmModeConnectorPtr conn;
   drmModePropertyPtr prop;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   output = (E_Drm_Output *)base;
   comp = base->compositor;
   dcomp = (E_Drm_Compositor *)comp;

   if (!(conn = drmModeGetConnector(dcomp->drm.fd, output->conn_id)))
     return;
   if (!(prop = e_drm_output_get_property(dcomp->drm.fd, conn, "DPMS")))
     {
        drmModeFreeConnector(conn);
        return;
     }
   drmModeConnectorSetProperty(dcomp->drm.fd, conn->connector_id, 
                               prop->prop_id, level);
   drmModeFreeProperty(prop);
   drmModeFreeConnector(conn);
}

EINTERN Eina_Bool 
e_drm_output_prepare_scanout_surface(E_Drm_Output *output)
{
   E_Drm_Compositor *dcomp;
   E_Surface *es;
   EGLint hdl, stride;
   int ret = 0;
   unsigned int fb_id = 0;
   struct gbm_bo *bo;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   dcomp = (E_Drm_Compositor *)output->base.compositor;
   es = container_of(dcomp->base.surfaces.next, E_Surface, link);

   /* Need to verify output->region contained in surface opaque
    * region.  Or maybe just that format doesn't have alpha. */
   return EINA_TRUE;

   if ((es->geometry.x != output->base.x) || 
       (es->geometry.y != output->base.y) || 
       (es->geometry.w != output->base.current->w) || 
       (es->geometry.h != output->base.current->h) || 
       (es->transform.enabled) || (es->image == EGL_NO_IMAGE_KHR))
     return EINA_FALSE;

   bo = gbm_bo_create_from_egl_image(dcomp->gbm, dcomp->base.egl_display, 
                                     es->image, es->geometry.w, 
                                     es->geometry.h, GBM_BO_USE_SCANOUT);
   hdl = gbm_bo_get_handle(bo).s32;
   stride = gbm_bo_get_pitch(bo);

   gbm_bo_destroy(bo);

   if (hdl == 0) return EINA_FALSE;

   ret = drmModeAddFB(dcomp->drm.fd, output->base.current->w, 
                      output->base.current->h, 24, 32, stride, hdl, &fb_id);
   if (ret) return EINA_FALSE;

   output->pending_fs_surf_fb_id = fb_id;

   output->pending_scanout_buffer = es->buffer;
   output->pending_scanout_buffer->busy_count++;

   wl_signal_add(&output->pending_scanout_buffer->resource.destroy_signal,
                  &output->pending_scanout_buffer_destroy_listener);

   pixman_region32_fini(&es->damage);
   pixman_region32_init(&es->damage);

   return EINA_TRUE;
}

EINTERN void 
e_drm_output_disable_sprites(E_Output *base)
{
   E_Compositor *comp;
   E_Drm_Compositor *dcomp;
   E_Drm_Output *output;
   E_Sprite *s;
   int ret = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = base->compositor;
   dcomp = (E_Drm_Compositor *)comp;
   output = (E_Drm_Output *)base;

   wl_list_for_each(s, &dcomp->sprites, link)
     {
        if (s->pending_fb_id) continue;
        ret = drmModeSetPlane(dcomp->drm.fd, s->plane_id, output->crtc_id, 
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        if (ret)
          printf("Failed to disable plane: %s\n", strerror(errno));
        drmModeRmFB(dcomp->drm.fd, s->fb_id);
        s->surface = NULL;
        s->pending_surface = NULL;
        s->fb_id = 0;
        s->pending_fb_id = 0;
     }
}

EINTERN drmModePropertyPtr 
e_drm_output_get_property(int fd, drmModeConnectorPtr conn, const char *name)
{
   drmModePropertyPtr props;
   int i = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   for (i = 0; i < conn->count_props; i++)
     {
        if (!(props = drmModeGetProperty(fd, conn->props[i])))
          continue;
        if (!strcmp(props->name, name)) return props;
        drmModeFreeProperty(props);
     }

   return NULL;
}

EINTERN int 
e_drm_output_prepare_overlay_surface(E_Output *base, E_Surface *es, pixman_region32_t *overlap)
{
   E_Compositor *comp;
   E_Drm_Compositor *dcomp;
   E_Sprite *s;
   Eina_Bool found = EINA_FALSE;
   EGLint hdl, stride;
   struct gbm_bo *bo;
   unsigned int fb_id = 0;
   unsigned int hdls[4], pitches[4], offsets[4];
   int ret = 0;
   pixman_region32_t drect, srect;
   pixman_box32_t *box;
   unsigned int format;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = base->compositor;
   dcomp = (E_Drm_Compositor *)comp;

   if (dcomp->sprites_broken) return -1;
   if (e_surface_is_primary(comp, es)) return -1;
   if (es->image == EGL_NO_IMAGE_KHR) return -1;

   if (!e_drm_output_surface_transform_supported(es))
     return -1;
   if (!e_drm_output_surface_overlap_supported(base, overlap))
     return -1;

   wl_list_for_each(s, &dcomp->sprites, link)
     {
        if (!e_sprite_crtc_supported(base, s->possible_crtcs))
          continue;
        if (!s->pending_fb_id)
          {
             found = EINA_TRUE;
             break;
          }
     }

   if (!found) return -1;

   bo = gbm_bo_create_from_egl_image(dcomp->gbm, dcomp->base.egl_display, 
                                     es->image, es->geometry.w, 
                                     es->geometry.h, GBM_BO_USE_SCANOUT);
   format = gbm_bo_get_format(bo);
   hdl = gbm_bo_get_handle(bo).s32;
   stride = gbm_bo_get_pitch(bo);

   gbm_bo_destroy(bo);

   if (!e_drm_output_surface_format_supported(s, format))
     return -1;

   if (!hdl) return -1;

   hdls[0] = hdl;
   pitches[0] = stride;
   offsets[0] = 0;

   ret = drmModeAddFB2(dcomp->drm.fd, es->geometry.w, es->geometry.h, 
                       format, hdls, pitches, offsets, &fb_id, 0);
   if (ret)
     {
        dcomp->sprites_broken = EINA_TRUE;
        return -1;
     }

   if ((s->surface) && (s->surface != es))
     {
        E_Surface *os;

        os = s->surface;
        pixman_region32_fini(&os->damage);
        pixman_region32_init_rect(&os->damage, os->geometry.x, os->geometry.y,
                                  os->geometry.w, os->geometry.h);
     }

   s->pending_fb_id = fb_id;
   s->pending_surface = es;
   es->buffer->busy_count++;

   pixman_region32_init(&drect);
   pixman_region32_intersect(&drect, &es->transform.box, &base->region);
   pixman_region32_translate(&drect, -base->x, -base->y);

   box = pixman_region32_extents(&drect);
   s->dx = box->x1;
   s->dy = box->y1;
   s->dw = (box->x2 - box->x1);
   s->dh = (box->y2 - box->y1);
   pixman_region32_fini(&drect);

   pixman_region32_init(&srect);
   pixman_region32_intersect(&srect, &es->transform.box, &base->region);
   pixman_region32_translate(&srect, -es->geometry.x, -es->geometry.y);

   box = pixman_region32_extents(&srect);
   s->sx = box->x1 << 16;
   s->sy = box->y1 << 16;
   s->sw = (box->x2 - box->x1) << 16;
   s->sh = (box->y2 - box->y1) << 16;
   pixman_region32_fini(&srect);

   wl_signal_add(&es->buffer->resource.destroy_signal,
                  &s->pending_destroy_listener);

   return 0;
}

EINTERN Eina_Bool 
e_drm_output_surface_transform_supported(E_Surface *es)
{
   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((es) && (es->transform.enabled)) return EINA_TRUE;
   return EINA_FALSE;
}

EINTERN Eina_Bool 
e_drm_output_surface_overlap_supported(E_Output *base __UNUSED__, pixman_region32_t *overlap)
{
   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (pixman_region32_not_empty(overlap)) return EINA_TRUE;
   return EINA_FALSE;
}

EINTERN Eina_Bool 
e_drm_output_surface_format_supported(E_Sprite *s, unsigned int format)
{
   unsigned int i = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   for (i = 0; i < s->format_count; i++)
     if (s->formats[i] == format) return EINA_TRUE;

   return EINA_FALSE;
}

EINTERN void 
e_drm_output_set_cursor_region(E_Output *output, E_Input_Device *device, pixman_region32_t *overlap)
{
   pixman_region32_t cregion;
   Eina_Bool prior = EINA_FALSE;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!device->sprite) return;
   pixman_region32_init(&cregion);
   pixman_region32_intersect(&cregion, &device->sprite->transform.box, 
                             &output->region);
   if (!pixman_region32_not_empty(&cregion))
     {
        e_drm_output_set_cursor(output, NULL);
        goto out;
     }

   prior = device->hw_cursor;
   if ((pixman_region32_not_empty(overlap)) || 
       (!e_drm_output_set_cursor(output, device)))
     {
        if (prior)
          {
             e_surface_damage(device->sprite);
             e_drm_output_set_cursor(output, NULL);
          }
        device->hw_cursor = EINA_FALSE;
     }
   else
     {
        if (!prior) e_surface_damage_below(device->sprite);
        pixman_region32_fini(&device->sprite->damage);
        pixman_region32_init(&device->sprite->damage);
        device->hw_cursor = EINA_TRUE;
     }

out:
   pixman_region32_fini(&cregion);
}

EINTERN Eina_Bool 
e_drm_output_set_cursor(E_Output *output, E_Input_Device *device)
{
   E_Drm_Output *doutput;
   E_Drm_Compositor *dcomp;
   EGLint hdl, stride;
   int ret = -1;
   struct gbm_bo *bo;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   doutput = (E_Drm_Output *)output;
   dcomp = (E_Drm_Compositor *)doutput->base.compositor;

   if (!device)
     {
        drmModeSetCursor(dcomp->drm.fd, doutput->crtc_id, 0, 0, 0);
        return EINA_TRUE;
     }

   if (device->sprite->image == EGL_NO_IMAGE_KHR) return EINA_FALSE;

   if ((device->sprite->geometry.w > 64) || 
       (device->sprite->geometry.h > 64))
     return EINA_FALSE;

   bo = gbm_bo_create_from_egl_image(dcomp->gbm, dcomp->base.egl_display, 
                                     device->sprite->image, 64, 64, 
                                     GBM_BO_USE_CURSOR_64X64);
   if (!bo) return EINA_FALSE;

   hdl = gbm_bo_get_handle(bo).s32;
   stride = gbm_bo_get_pitch(bo);
   gbm_bo_destroy(bo);

   if (stride != (64 * 4)) return EINA_FALSE;

   if ((ret = drmModeSetCursor(dcomp->drm.fd, doutput->crtc_id, hdl, 64, 64)))
     {
        drmModeSetCursor(dcomp->drm.fd, doutput->crtc_id, 0, 0, 0);
        return EINA_FALSE;
     }

   ret = drmModeMoveCursor(dcomp->drm.fd, doutput->crtc_id, 
                           device->sprite->geometry.x - doutput->base.x,
                           device->sprite->geometry.y - doutput->base.y);
   if (ret)
     {
        drmModeSetCursor(dcomp->drm.fd, doutput->crtc_id, 0, 0, 0);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}
