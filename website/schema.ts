import {sqliteTable,text,integer,primaryKey} from 'drizzle-orm/sqlite-core';
export const downloadDaily=sqliteTable('download_daily',{
 day:text('day').notNull(),fileId:text('file_id').notNull(),requests:integer('requests').notNull().default(0)
},t=>[primaryKey({columns:[t.day,t.fileId]})]);
export const newsletterSubscribers=sqliteTable('newsletter_subscribers',{
 email:text('email').primaryKey().notNull(),status:text('status').notNull(),consentVersion:text('consent_version').notNull(),consentedAt:text('consented_at').notNull(),confirmedAt:text('confirmed_at'),confirmationHash:text('confirmation_hash'),confirmationExpiresAt:text('confirmation_expires_at'),unsubscribeHash:text('unsubscribe_hash').notNull(),mailSentAt:text('mail_sent_at'),updatedAt:text('updated_at').notNull()
});
export const newsletterSendBudget=sqliteTable('newsletter_send_budget',{
 minute:text('minute').primaryKey().notNull(),requests:integer('requests').notNull().default(0)
});
